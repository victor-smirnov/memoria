
// Copyright 2019-2022 Victor Smirnov
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

#include <memoria/core/linked/common/arena.hpp>
#include <memoria/core/linked/common/linked_vector.hpp>
#include <memoria/core/linked/common/linked_tools.hpp>
#include <memoria/core/tools/optional.hpp>

#include <memoria/core/hermes/arena.hpp>
#include <memoria/core/hermes/array.hpp>

#include <memoria/core/hermes/hash_fn.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/tools/optional.hpp>

#include <functional>
#include <deque>
#include <cstddef>

namespace memoria {
namespace arena {

template <
    typename Key,
    typename Value
>
class Map {
    using Hash = DefaultHashFn<Key>;
    using KeyEqual  = DefaultEqualToFn<Key>;

    uint64_t size_;
    uint64_t buckets_capacity_;
    SegmentPtr<uint64_t> buckets_;

    struct alignas(Key) alignas(Value) alignas(uint32_t) Bucket {
        static constexpr bool UseObjectSize = true;

        uint32_t size;
        uint32_t capacity;

        Bucket(size_t capacity) noexcept:
            capacity(capacity)
        {}

        Key* keys() noexcept {
            return ptr_cast<Key>(reinterpret_cast<uint8_t*>(this) + HEADER_SIZE);
        }

        const Key* keys() const noexcept {
            return ptr_cast<const Key>(reinterpret_cast<const uint8_t*>(this) + HEADER_SIZE);
        }

        Value* values() noexcept
        {
            uint8_t* base = reinterpret_cast<uint8_t*>(this) + HEADER_SIZE;
            size_t keys_size = keys_block_size(capacity);
            return ptr_cast<Value>(base + keys_size);
        }

        const Value* values() const noexcept
        {
            const uint8_t* base = reinterpret_cast<const uint8_t*>(this) + HEADER_SIZE;
            size_t keys_size = keys_block_size(capacity);
            return ptr_cast<const Value>(base + keys_size);
        }

        static size_t object_size(size_t capacity) noexcept
        {
            size_t kb_size = keys_block_size(capacity);
            size_t values_size = sizeof(Value) * capacity;
            return HEADER_SIZE + kb_size + values_size;
        }

        uint32_t find(MemorySegment* sgm, const Key& key) const noexcept
        {
            KeyEqual eq;
            const Key* keys = this->keys();
            for (uint32_t c = 0; c < size; c++) {
                if (eq(sgm, key, keys[c])) {
                    return c;
                }
            }

            return size;
        }

        void remove(uint32_t idx)
        {
            Key* keys = this->keys();
            Value* values = this->values();

            for (uint32_t c = idx + 1; c < size; c++)
            {
                keys[c - 1] = keys[c];
                values[c - 1] = values[c];
            }

            size -= 1;
        }

    private:
        static  size_t keys_block_size(size_t capacity)
        {
            constexpr size_t mask = values_alignment_mask();
            size_t data_size = sizeof(Key) * capacity;

            if ((data_size & mask) == 0) {
                return data_size;
            }
            else {
                return (data_size | mask) + 1;
            }
        }

        static constexpr size_t HEADER_SIZE = 8;
    };

    friend struct Bucket;
    friend class Iterator;

public:
    Map() noexcept :
        size_(), buckets_capacity_()
    {}

    class Iterator {
        SegmentPtr<Map> map_ptr_;
        size_t bucket_idx_;
        SegmentPtr<Bucket> bucket_ptr_;
        uint32_t entry_idx_;
        uint32_t bucket_size_;
    public:
        Iterator(
                SegmentPtr<Map> map_ptr,
                size_t bucket_idx, SegmentPtr<Bucket> bucket_ptr,
                uint32_t entry_idx, uint32_t bucket_size
        ) noexcept:
            map_ptr_(map_ptr), bucket_idx_(bucket_idx),
            bucket_ptr_(bucket_ptr), entry_idx_(entry_idx),
            bucket_size_(bucket_size)
        {}

        bool next(const MemorySegment* sgm) noexcept
        {
            const Map* map = map_ptr_.read(sgm);
            size_t capacity = 1ull << map->buckets_capacity_;

            if (entry_idx_ < bucket_size_ - 1) {
                entry_idx_++;
                return true;
            }
            else if (bucket_idx_ < capacity)
            {
                bucket_idx_++;
                entry_idx_ = 0;

                while (bucket_idx_ < capacity)
                {
                    bucket_ptr_ = SegmentPtr<Bucket>::from(*map->buckets_.read(sgm, bucket_idx_));
                    if (!bucket_ptr_.is_null())
                    {
                        const Bucket* bucket = bucket_ptr_.read(sgm);
                        bucket_size_ = bucket->size;
                        return true;
                    }
                    else {
                        bucket_idx_++;
                    }
                }
            }

            return false;
        }

        Key key(const MemorySegment* sgm) const {
            const Bucket* bucket = bucket_ptr_.read(sgm);
            return bucket->keys()[entry_idx_];
        }

        Value value(const MemorySegment* sgm) const {
            const Bucket* bucket = bucket_ptr_.read(sgm);
            return bucket->values()[entry_idx_];
        }
    };


    uint64_t size() const {
        return size_;
    }

    Optional<Iterator> iterator(const MemorySegment* sgm) const noexcept
    {
        if (size_) {
            SegmentPtr<Map> self_ptr = SegmentPtr<Map>::from(sgm, this);

            size_t bucket_idx{};
            size_t capacity = 1ull << buckets_capacity_;

            while (bucket_idx < capacity)
            {
                auto bucket_ptr = SegmentPtr<Bucket>::from(*this->buckets_.read(sgm, bucket_idx));

                if (!bucket_ptr.is_null()) {
                    const Bucket* bucket = bucket_ptr.read(sgm);
                    return Iterator(
                        self_ptr, bucket_idx, bucket_ptr, 0, bucket->size
                    );
                }

                bucket_idx++;
            }
        }

        return Optional<Iterator>{};
    }

    Optional<Value> get(const MemorySegment* sgm, const Key& key) const
    {
        if (size_)
        {
            Hash hh;
            uint64_t bucket_idx = to_bucket(hh(sgm, key));
            SegmentPtr<Bucket> bucket_ptr = SegmentPtr<Bucket>::from(
                    *buckets_.read(sgm, bucket_idx)
            );

            if (bucket_ptr)
            {
                const Bucket* bucket = bucket_ptr.read(sgm);
                const Key* keys = bucket->keys();

                uint32_t idx = bucket->find(key);
                if (idx < bucket->size) {
                    return bucket->values()[idx];
                }
            }
        }

        return Optional<Value>{};
    }

    Map* put(ArenaSegment* arena, const Key& key, const Value& value)
    {
        if (MMA_UNLIKELY(buckets_.is_null())) {
            return put_empty(arena, key, value);
        }
        else if (size_ * 1 > (1ull << buckets_capacity_) * 2)
        {
            Map* self = enlarge_array(this, arena);
            return self->put(arena, key, value);
        }
        else {
            bool replace{true};
            Map* self = insert_into_array(arena, this, buckets_, buckets_capacity_, key, value, replace);
            // This value is changing in the insert_into_array()
            if (!replace) {
                self->size_++;
            }
            return self;
        }
    }

    Map* remove(ArenaSegment* arena, const Key& key)
    {
        Map* self = this;

        if (MMA_LIKELY(size_))
        {
            size_t buckets_capacity = 1ull << buckets_capacity_;

            if (size_ * 2 > buckets_capacity)
            {
                Hash hh;
                uint64_t bucket_idx = to_bucket(hh(arena, key));
                SegmentPtr<Bucket> bucket_ptr = SegmentPtr<Bucket>::from(
                    *self->buckets_.read(arena, bucket_idx)
                );

                if (!bucket_ptr.is_null())
                {
                    Bucket* bucket = bucket_ptr.write(arena);
                    uint32_t idx = bucket->find(arena, key);

                    if (idx < bucket->size)
                    {
                        bucket->remove(idx);

                        if (self->size_ > 1)
                        {
                            if (bucket->size <= bucket->capacity / 2)
                            {
                                SegmentPtr<Map> self_ptr = SegmentPtr<Map>::from(arena, self);
                                auto state = arena->state();

                                SegmentPtr<uint64_t> buckets_ptr = self->buckets_;

                                SegmentPtr<Bucket> new_bucket_ptr = self->resize_bucket(arena, bucket_ptr, bucket->capacity / 2);
                                *buckets_ptr.write(arena, bucket_idx) = new_bucket_ptr.offset();

                                state.on_enlarge([&]{
                                    self = self_ptr.write(arena);
                                });
                            }
                        }
                        else {
                            self->buckets_.reset();
                            self->buckets_capacity_ = 0;
                        }

                        self->size_--;
                    }
                }
            }
            else {
                self = shrink_array(self, arena);
                self = self->remove(arena, key);
            }
        }

        return self;
    }

    template <typename Fn>
    void for_each(const MemorySegment* sgm, Fn&& fn) const
    {
        Optional<Iterator> ii = this->iterator(sgm);
        if (ii.is_initialized()) {
            do {
                fn(ii.get().key(sgm), ii.get().value(sgm));
            }
            while (ii.get().next(sgm));
        }
    }
    
private:

    Map* put_empty(ArenaSegment* arena, const Key& key, const Value& value)
    {
        auto state = arena->state();
        SegmentPtr<Map> map_ptr = SegmentPtr<Map>::from(arena, this);
        Map* self = this;

        auto buckets_ptr = arena->template allocate_space<uint64_t>(2 * sizeof(uint64_t));
        state.on_enlarge([&]{
            self = map_ptr.write(arena);
        });

        self->buckets_ = buckets_ptr;
        self->buckets_capacity_ = 1;

        bool replace{};
        self = self->insert_into_array(arena, self, buckets_ptr, 1, key, value, replace);

        self->size_ = 1;

        return self;
    }

    static Map* enlarge_array(Map* self, ArenaSegment* arena)
    {
        auto state = arena->state();
        SegmentPtr<Map> self_ptr = SegmentPtr<Map>::from(arena, self);

        SegmentPtr<uint64_t> buckets_ptr = self->buckets_;

        size_t buckets_capacity = self->buckets_capacity_;

        auto new_buckets_ptr = arena->template allocate_space<uint64_t>((1ull << buckets_capacity) * 2 * sizeof(uint64_t));
        state.on_enlarge([&]{
            self = self_ptr.write(arena);
        });

        self = self->copy_buckets(arena, self, buckets_ptr, buckets_capacity, new_buckets_ptr, buckets_capacity + 1);
        self->buckets_ = new_buckets_ptr;
        self->buckets_capacity_ = buckets_capacity + 1;

        return self;
    }


    static Map* shrink_array(Map* self, ArenaSegment* arena)
    {
        if (self->size_)
        {
            auto state = arena->state();
            SegmentPtr<Map> self_ptr = SegmentPtr<Map>::from(arena, self);

            SegmentPtr<uint64_t> buckets_ptr = self->buckets_;

            size_t buckets_capacity = self->buckets_capacity_;

            auto new_buckets_ptr = arena->template allocate_space<uint64_t>((1ull << (buckets_capacity - 1)) * sizeof(uint64_t));
            state.on_enlarge([&]{
                self = self_ptr.write(arena);
            });

            self = self->copy_buckets(arena, self, buckets_ptr, buckets_capacity, new_buckets_ptr, buckets_capacity - 1);

            self->buckets_ = new_buckets_ptr;
            self->buckets_capacity_ = buckets_capacity - 1;
            return self;
        }
        else {
            self->buckets_ = SegmentPtr<uint64_t>{};
            self->buckets_capacity_ = 0;
            return self;
        }
    }

    static Map* copy_buckets(
            ArenaSegment* arena,
            Map* self,
            SegmentPtr<uint64_t> src_buckets_ptr,
            size_t src_buckets_capacity,
            SegmentPtr<uint64_t> dst_buckets_ptr,
            size_t dst_buckets_capacity
    )
    {
        for (size_t c = 0; c < 1ull << src_buckets_capacity; c++)
        {
            SegmentPtr<Bucket> bucket_ptr = SegmentPtr<Bucket>::from(
                *src_buckets_ptr.read(arena, c)
            );

            if (!bucket_ptr.is_null())
            {
                uint32_t bucket_size = bucket_ptr.read(arena)->size;
                for (size_t d = 0; d < bucket_size; d++)
                {
                    const Bucket* bucket = bucket_ptr.read(arena);

                    Key key = bucket->keys()[d];
                    Value value = bucket->values()[d];

                    bool replace{};
                    self = insert_into_array(arena, self, dst_buckets_ptr, dst_buckets_capacity, key, value, replace);
                }
            }
        }

        return self;
    }

    uint64_t to_bucket(uint64_t hash) const {
        return to_bucket(buckets_capacity_, hash);
    }

    uint64_t to_bucket(size_t capacity, uint64_t hash) const {
        return hash & ((1ull << capacity) - 1);
    }

    SegmentPtr<Bucket> resize_bucket(ArenaSegment* arena, SegmentPtr<Bucket> bucket_ptr, uint32_t new_capacity)
    {
        if (MMA_LIKELY(new_capacity))
        {
            SegmentPtr<Bucket> new_bucket_ptr = arena->allocate_untagged<Bucket>(new_capacity);

            Bucket* b1 = bucket_ptr.write(arena);
            Bucket* new_bucket = new_bucket_ptr.write(arena);

            new_bucket->size = b1->size;

            Key* b1_keys = b1->keys();
            Value* b1_values = b1->values();

            Key* new_keys = new_bucket->keys();
            Value* new_values = new_bucket->values();

            for (size_t c = 0; c < b1->size; c++) {
                new_keys[c] = b1_keys[c];
                new_values[c] = b1_values[c];
            }

            return new_bucket_ptr;
        }
        else {
            return SegmentPtr<Bucket>{};
        }
    }


    static SegmentPtr<Bucket> create_bucket(ArenaSegment* arena) {
        return arena->allocate_untagged<Bucket>(1);
    }

    static constexpr uint64_t values_alignment_mask() noexcept
    {
        size_t align = alignof(Value);
        return align - 1;
    }

    static Map* insert_into_array(
            ArenaSegment* arena,
            Map* self,
            SegmentPtr<uint64_t> buckets_ptr,
            size_t capacity,
            Key key,
            Value value,
            bool& replace
    )
    {
        Hash hh;
        uint64_t bucket_idx = self->to_bucket(capacity, hh(arena, key));
        SegmentPtr<Bucket> bucket_ptr = SegmentPtr<Bucket>::from(
                *buckets_ptr.read(arena, bucket_idx)
        );

        SegmentPtr<Map> self_ptr = SegmentPtr<Map>::from(arena, self);

        if (!bucket_ptr.is_null())
        {
            Bucket* bucket = bucket_ptr.write(arena);
            Key* keys = bucket->keys();

            uint32_t idx;
            if (replace)
            {
                idx = bucket->find(arena, key);
                if (idx < bucket->size) {
                    bucket->values()[idx] = value;
                    return self;
                }
                else {
                    replace = false;
                }
            }
            else {
                idx = bucket->size;
            }

            if (idx < bucket->capacity)
            {
                bucket->size++;
                keys[idx] = key;
                bucket->values()[idx] = value;
                return self;
            }
            else {
                auto state = arena->state();

                SegmentPtr<Bucket> new_bucket_ptr = self->resize_bucket(arena, bucket_ptr, bucket->capacity * 2);
                Bucket* new_bucket = new_bucket_ptr.write(arena);
                *buckets_ptr.write(arena, bucket_idx) = new_bucket_ptr.offset();

                new_bucket->size++;
                Key* new_keys = new_bucket->keys();
                Value* new_values = new_bucket->values();

                new_keys[idx] = key;
                new_values[idx] = value;

                state.on_enlarge([&]{
                    self = self_ptr.write(arena);
                });

                return self;
            }
        }
        else {
            replace = false;
            auto state = arena->state();

            SegmentPtr<Bucket> new_bucket_ptr = self->create_bucket(arena);
            state.on_enlarge([&]{
                self = self_ptr.write(arena);
            });

            Bucket* new_bucket = new_bucket_ptr.write(arena);
            new_bucket->size = 1;

            Key* new_keys = new_bucket->keys();
            Value* new_values = new_bucket->values();

            *new_keys = key;
            *new_values = value;

            *buckets_ptr.write(arena, bucket_idx) = new_bucket_ptr.offset();

            return self;
        }
    }

public:
    void dump_state(const MemorySegment* sgm) const
    {
        size_t array_size = 1ull << buckets_capacity_;
        println("Map[{}, {}]{{", size_, array_size);

        if (!buckets_.is_null())
        {
            for (size_t c = 0; c < array_size; c++)
            {
                SegmentPtr<Bucket> bucket_ptr = SegmentPtr<Bucket>::from(
                    *buckets_.read(sgm, c)
                );

                if (!bucket_ptr.is_null())
                {
                    const Bucket* bucket = bucket_ptr.read(sgm);
                    print("\t{}: <",c);

                    auto keys   = bucket->keys();
                    auto values = bucket->values();

                    for (uint32_t b = 0; b < bucket->size; b++) {
                        print("[{}, {}]", keys[b], values[b]);
                        if (b < bucket->size - 1) {
                            print(", ");
                        }
                    }
                    println(">");
                }
                else {
                    println("\t{}: null", c);
                }
            }
        }

        println("}}");
    }
};


}}
