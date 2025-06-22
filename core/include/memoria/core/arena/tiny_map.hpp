
// Copyright 2019-2025 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <memoria/core/types.hpp>

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>


#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/bitmap_select.hpp>
#include <memoria/core/reflection/reflection.hpp>



#include <functional>

namespace memoria {
namespace arena {

namespace detail {

template <typename T>
struct EmptyValueProvider {
    static T make_empty() {
        return T{};
    }
};

template <typename T>
struct EmptyValueProvider<EmbeddingRelativePtr<T>> {
    static EmbeddingRelativePtr<T> make_empty() {
        return EmbeddingRelativePtr<T>{static_cast<T*>(0)};
    }
};

template <typename T>
struct EmptyValueProvider<RelativePtr<T>> {
    static RelativePtr<T> make_empty() {
        return RelativePtr<T>{static_cast<T*>(0)};
    }
};


}


template <typename, typename> class Map;

template <typename Value>
class Map<uint8_t, Value> {
    static constexpr uint64_t SIZE_BITS = 6;
    static constexpr uint64_t MAX_BITS  = 64;
    static constexpr uint64_t SIZE_BITS_MASK = 0x3F;

    static constexpr uint64_t SIZE_START     = MAX_BITS - SIZE_BITS;
    static constexpr uint64_t CAPACITY_START = MAX_BITS - SIZE_BITS * 2;

    static constexpr uint64_t MAX_KEYS = CAPACITY_START; // Should be 64 - 6 - 6 = 52
    static constexpr uint64_t BITMAP_MASK = ~((SIZE_BITS_MASK << SIZE_START) | (SIZE_BITS_MASK << CAPACITY_START));

    uint64_t header_;
    RelativePtr<Value> data_;

public:    
    class Iterator {
        const Map* map_;
        uint64_t entry_idx_;
    public:
        Iterator(
                const Map* map,
                uint64_t entry_idx
        ) noexcept:
            map_(map), entry_idx_(entry_idx)
        {}

        bool operator==(const Iterator& other) const {
            return map_ == other.map_ && entry_idx_ == other.entry_idx_;
        }

        bool operator!=(const Iterator& other) const {
            return map_ != other.map_ || entry_idx_ != other.entry_idx_;
        }

        bool next() noexcept
        {
            auto size = map_->size();

            if (entry_idx_ < size) {
                entry_idx_++;
                return true;
            }

            return false;
        }

        const uint8_t key() const
        {
            uint64_t bits = map_->header_ & BITMAP_MASK;
            uint64_t key  = SelectFW(bits, entry_idx_ + 1);
            return key;
        }

        const Value& value() const {
            return map_->data()[entry_idx_];
        }
    };

    friend class Iterator;

    Map():
        header_(0), data_()
    {}

    Map(uint64_t header, uint64_t capacity):
        header_(header), data_()
    {
        set_capacity(capacity);
    }

    Iterator begin() const {
        return Iterator(this, 0);
    }

    Iterator end() const {
        return Iterator(this, size());
    }

    uint64_t size() const noexcept {
        return header_ >> SIZE_START;
    }

    uint64_t capacity() const noexcept {
        return (header_ >> CAPACITY_START) & SIZE_BITS_MASK;
    }

    const Value* get(uint8_t key, LWMemHolder*) const noexcept
    {
        if (MMA_LIKELY(key < MAX_KEYS))
        {
            uint64_t key_mask = 1ull << key;
            if (MMA_LIKELY(header_ & key_mask))
            {
                uint64_t mask = make_bitmask<uint64_t>(key);
                uint64_t pos = PopCnt2(header_ & mask);
                if (pos < size()) {
                    return &data()[pos];
                }
            }
        }

        return nullptr;
    }

    void put(ArenaAllocator& arena, uint8_t key, const Value& value, LWMemHolder*)
    {
        if (key < MAX_BITS)
        {
            uint64_t key_mask = 1ull << key;
            if (MMA_LIKELY(!(header_ & key_mask)))
            {
                uint64_t size = this->size();
                uint64_t capacity = this->capacity();

                uint64_t mask = make_bitmask<uint64_t>(key);
                uint64_t bitmap = header_ & mask;
                uint64_t pos = PopCnt2(bitmap);

                if (size < capacity)
                {
                    header_ |= key_mask;
                    insert_space(pos);
                    data()[pos] = value;
                    set_size(size + 1);
                }
                else {
                    uint64_t new_capacity = capacity > 0 ? (capacity * 2) : 1;
                    if (new_capacity > MAX_KEYS) {
                        new_capacity = MAX_KEYS;
                    }

                    Value* new_data = arena.allocate_untagged_array<Value>(new_capacity);
                    header_ |= key_mask;
                    set_size(size + 1);
                    set_capacity(new_capacity);

                    Value* data = this->data();
                    for (uint64_t c = 0; c < pos; c++) {
                        new_data[c] = data[c];
                    }

                    for (uint64_t c = pos; c < size; c++) {
                        new_data[c + 1] = data[c];
                    }

                    new_data[pos] = value;
                    data_ = new_data;
                }
            }
            else {
                uint64_t mask = make_bitmask<uint64_t>(key);
                uint64_t pos = PopCnt2(header_ & mask);
                data()[pos] = value;
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Key value {} exceeds max Map<uint8_t, T> range of [0, {})", (int)key, MAX_KEYS).do_throw();
        }
    }

    void remove(ArenaAllocator& arena, uint8_t key, LWMemHolder*) noexcept
    {
        if (key < MAX_KEYS)
        {
            uint64_t key_mask = 1ull << key;
            if (MMA_LIKELY(header_ & key_mask))
            {
                uint64_t size = this->size();
                uint64_t capacity = this->capacity();

                uint64_t mask = make_bitmask<uint64_t>(key);
                uint64_t pos  = PopCnt2(header_ & mask);

                if (MMA_LIKELY(size - 1 > capacity / 2))
                {
                    header_ &= ~key_mask;
                    remove_space(pos);
                    set_size(size - 1);
                }
                else {
                    header_ &= ~key_mask;

                    uint64_t new_capacity = capacity / 2;
                    if (new_capacity)
                    {
                        set_capacity(new_capacity);
                        set_size(size - 1);

                        Value* new_data = arena.allocate_untagged_array<Value>(new_capacity);

                        Value* data = this->data();
                        for (uint64_t c = 0; c < pos; c++) {
                            new_data[c] = data[c];
                        }

                        for (uint64_t c = pos + 1; c < size; c++) {
                            new_data[c - 1] = data[c];
                        }

                        data_ = new_data;
                    }
                    else {
                        data_ = nullptr;
                        set_capacity(0);
                        set_size(0);
                    }
                }
            }
        }
    }

    Value* data() {
        return data_.get();
    }

    const Value* data() const {
        return data_.get();
    }

    template <typename Fn>
    void for_each(Fn&& fn) const
    {
        Iterator ii = begin();
        Iterator end = this->end();

        while (ii != end) {
            fn(ii.key(), ii.value());
            ii.next();
        }
    }

    Map* deep_copy_to(
            ShortTypeCode tag,
            hermes::DeepCopyState& dedup) const
    {
        auto& dst = dedup.arena();
        Map* existing = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)existing)) {
            return existing;
        }
        else {
            auto size = this->size();
            uint64_t new_capacity = size;

            auto map = dst.get_resolver_for(dst.template allocate_tagged_object<Map>(tag, header_, new_capacity));
            dedup.map(dst, this, map.get(dst));
            map.get(dst)->set_size(size);

            if (new_capacity)
            {
                auto data = dst.get_resolver_for(
                    dst.template allocate_untagged_array<Value>(new_capacity)
                );

                map.get(dst)->data_ = data.get(dst);

                const Value* src_data = this->data();
                memoria::detail::DeepCopyHelper<Value>::deep_copy_to(
                    data, src_data, size, dedup
                );
            }
            else {
                map.get(dst)->data_ = nullptr;
            }

            return map.get(dst);
        }
    }

    void check_typed_map(hermes::CheckStructureState& state) const
    {
        if (size() <= capacity())
        {
            state.mark_as_processed(this);
            state.check_and_set_tagged(this, sizeof(Map), MA_SRC);

            const Value* data = this->data();

            for (size_t c = 0; c < size(); c++) {
                state.check_ptr(data[c], MA_SRC);
            }

            for (size_t c = size(); c < capacity(); c++) {
                if (data[c].is_not_null()) {
                    MEMORIA_MAKE_GENERIC_ERROR("Map<UTinyInt, Object> has non_null empty slot at {}", c).do_throw();
                }
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Map<UTinyInt, Object> size/capacity check error: {} {}", size(), capacity()).do_throw();
        }
    }

private:

    void set_size(uint64_t value) noexcept
    {
        header_ &= ~(SIZE_BITS_MASK << SIZE_START);
        header_ |= (value << SIZE_START);
    }

    void set_capacity(uint64_t value) noexcept
    {
        header_ &= ~(SIZE_BITS_MASK << CAPACITY_START);
        header_ |= ((value & SIZE_BITS_MASK) << CAPACITY_START);
    }

    void insert_space(uint64_t idx)
    {
        Value* data   = this->data();
        uint64_t size = this->size();

        for (uint64_t c = size; c > idx; c--) {
            data[c] = data[c - 1];
        }
    }

    void remove_space(uint64_t idx)
    {
        Value* data   = this->data();
        uint64_t size = this->size();

        for (uint64_t c = idx + 1; c < size; c--) {
            data[c - 1] = data[c];
        }

        data[size] = detail::EmptyValueProvider<Value>::make_empty();
    }


    uint64_t usage_bits() const noexcept {
        return header_ & (SIZE_BITS_MASK << (MAX_BITS - SIZE_BITS));
    }

};

}}
