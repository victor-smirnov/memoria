
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

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>
#include <memoria/core/arena/hash_fn.hpp>
#include <memoria/core/arena/vector.hpp>

#include <memoria/core/hermes/traits.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/tools/optional.hpp>

#include <memoria/core/reflection/typehash.hpp>
#include <memoria/core/reflection/reflection.hpp>

#include <memoria/core/types/algo/select.hpp>

#include <functional>
#include <deque>
#include <cstddef>

namespace memoria {
namespace arena {

namespace detail {

template <typename T>
struct ElementHolderHelper {
    static T& resolve(T& e) noexcept {
        return e;
    }
};

template <typename T>
struct ElementHolderHelper<RelativePtr<T>> {
    static T* resolve(RelativePtr<T>& e) noexcept {
        return e.get();
    }
};

template <typename T>
struct ElementHolderHelper<EmbeddingRelativePtr<T>> {
    static T* resolve(EmbeddingRelativePtr<T>& e) noexcept {
        return e.get();
    }
};

}

template <
    typename Key,
    typename Value
>
class Map {

    using KeyHolder   = IfThenElse<std::is_pointer_v<Key>, RelativePtr<std::remove_pointer_t<Key>>, Key>;
    using ValueHolder = IfThenElse<std::is_pointer_v<Value>, EmbeddingRelativePtr<std::remove_pointer_t<Value>>, Value>;

    template <typename K>
    using Hash = DefaultHashFn<K>;

    using KeyEqual = DefaultEqualToFn<KeyHolder>;

    struct Bucket;
    using  BucketRelPtr = RelativePtr<Bucket>;

    uint64_t size_;
    uint64_t buckets_capacity_;
    RelativePtr<BucketRelPtr> buckets_;

    struct alignas(KeyHolder) alignas(ValueHolder) alignas(uint32_t) Bucket {
        static constexpr bool UseObjectSize = true;

        uint32_t size;
        uint32_t capacity;

        Bucket(size_t capacity) noexcept:
            size(0),
            capacity(capacity)
        {}

        size_t size_of() const noexcept {
            return object_size(capacity);
        }

        KeyHolder* keys() noexcept {
            return ptr_cast<KeyHolder>(reinterpret_cast<uint8_t*>(this) + HEADER_SIZE);
        }

        const KeyHolder* keys() const noexcept {
            return ptr_cast<const KeyHolder>(reinterpret_cast<const uint8_t*>(this) + HEADER_SIZE);
        }

        ValueHolder* values() noexcept
        {
            uint8_t* base = reinterpret_cast<uint8_t*>(this) + HEADER_SIZE;
            size_t keys_size = keys_block_size(capacity);
            return ptr_cast<ValueHolder>(base + keys_size);
        }

        const ValueHolder* values() const noexcept
        {
            const uint8_t* base = reinterpret_cast<const uint8_t*>(this) + HEADER_SIZE;
            size_t keys_size = keys_block_size(capacity);
            return ptr_cast<const ValueHolder>(base + keys_size);
        }

        static size_t object_size(size_t capacity) noexcept
        {
            size_t kb_size = keys_block_size(capacity);
            size_t values_size = sizeof(Value) * capacity;
            return HEADER_SIZE + kb_size + values_size;
        }

        template <typename KeyArg>
        uint32_t find(const KeyArg& key) const noexcept
        {
            KeyEqual eq;
            const KeyHolder* keys = this->keys();
            for (uint32_t c = 0; c < size; c++) {
                if (eq(key, keys[c])) {
                    return c;
                }
            }

            return size;
        }

        void remove(uint32_t idx)
        {
            KeyHolder* keys = this->keys();
            ValueHolder* values = this->values();

            for (uint32_t c = idx + 1; c < size; c++)
            {
                detail::CopyHelper<KeyHolder>::copy(keys[c - 1], keys[c]);
                detail::CopyHelper<ValueHolder>::copy(values[c - 1], values[c]);
            }

            size -= 1;
        }

    private:
        static size_t keys_block_size(size_t capacity)
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
        const Map* map_;
        size_t bucket_idx_;
        const Bucket* bucket_;
        uint32_t entry_idx_;
        uint32_t bucket_size_;
    public:
        Iterator(
                const Map* map,
                size_t bucket_idx, const Bucket* bucket,
                uint32_t entry_idx, uint32_t bucket_size
        ) noexcept:
            map_(map), bucket_idx_(bucket_idx),
            bucket_(bucket), entry_idx_(entry_idx),
            bucket_size_(bucket_size)
        {}

        bool operator==(const Iterator& other) const {
            return map_ == other.map_ && bucket_idx_ == other.bucket_idx_ && entry_idx_ == other.entry_idx_;
        }

        bool operator!=(const Iterator& other) const {
            return map_ != other.map_ || bucket_idx_ != other.bucket_idx_ || entry_idx_ != other.entry_idx_;
        }

        bool next() noexcept
        {
            size_t capacity = 1ull << map_->buckets_capacity_;

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
                    if (!map_->is_bucket_null(bucket_idx_))
                    {
                        bucket_ = map_->get_bucket(bucket_idx_);
                        bucket_size_ = bucket_->size;
                        return true;
                    }
                    else {
                        bucket_idx_++;
                    }
                }
            }

            return false;
        }

        const KeyHolder& key() const {
            return bucket_->keys()[entry_idx_];
        }

        const ValueHolder& value() const {
            return bucket_->values()[entry_idx_];
        }
    };


    uint64_t size() const {
        return size_;
    }


    Iterator begin() const noexcept
    {
        size_t capacity = 1ull << buckets_capacity_;

        if (size_)
        {
            size_t bucket_idx {};
            while (bucket_idx < capacity)
            {
                if (!this->is_bucket_null(bucket_idx))
                {
                    const Bucket* bucket = this->get_bucket(bucket_idx);
                    return Iterator(
                        this, bucket_idx, bucket, 0, bucket->size
                    );
                }

                bucket_idx++;
            }
        }

        return Iterator(this, capacity, nullptr, 0, 0);
    }

    Iterator end() const noexcept {
        size_t capacity = 1ull << buckets_capacity_;
        return Iterator(
            this, capacity, nullptr, 0, 0
        );
    }

    template <typename KeyArg>
    const ValueHolder* get(const KeyArg& key) const
    {
        if (size_)
        {
            Hash<KeyArg> hh;
            uint64_t bucket_idx = to_bucket(hh(key));
            if (!is_bucket_null(bucket_idx))
            {
                const Bucket* bucket = get_bucket(bucket_idx);
                uint32_t idx = bucket->find(key);
                if (idx < bucket->size) {
                    return &bucket->values()[idx];
                }
            }
        }

        return nullptr;
    }

    void put(ArenaAllocator& arena, const Key& key, const Value& value)
    {
        if (MMA_UNLIKELY(buckets_.is_null())) {
            return put_empty(arena, key, value);
        }
        else if (size_ > (1ull << buckets_capacity_))
        {
            enlarge_array(arena);
            return put(arena, key, value);
        }
        else {
            bool replace{true};
            insert_into_array(arena, buckets_.get(), buckets_capacity_, key, value, replace);
            // This value is changing in the insert_into_array()
            if (!replace) {
                size_++;
            }
        }
    }

    template <typename KeyArg>
    void remove(ArenaAllocator& arena, const KeyArg& key)
    {
        if (MMA_LIKELY(size_))
        {
            size_t buckets_capacity = 1ull << buckets_capacity_;
            if (size_ > buckets_capacity || buckets_capacity == 2)
            {
                Hash<KeyArg> hh;
                uint64_t bucket_idx = to_bucket(hh(key));

                if (!is_bucket_null(bucket_idx))
                {
                    Bucket* bucket = get_bucket(bucket_idx);

                    uint32_t idx = bucket->find(key);

                    if (idx < bucket->size)
                    {
                        bucket->remove(idx);
                        if (size_ > 1)
                        {
                            if (bucket->size <= bucket->capacity / 2)
                            {
                                Bucket* new_bucket = this->resize_bucket(arena, bucket, bucket->capacity / 2);
                                buckets_.get()[bucket_idx] = new_bucket;
                            }
                        }
                        else {
                            this->buckets_.reset();
                            this->buckets_capacity_ = 0;
                        }

                        this->size_--;
                    }
                }
            }
            else {
                shrink_array(arena);
                this->remove(arena, key);
            }
        }
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
    
private:

    void put_empty(ArenaAllocator& arena, const Key& key, const Value& value)
    {
        BucketRelPtr* buckets = arena.template allocate_untagged_array<BucketRelPtr>(2);

        this->buckets_ = buckets;
        this->buckets_capacity_ = 1;

        bool replace{};
        this->insert_into_array(arena, buckets, 1, key, value, replace);

        this->size_ = 1;
    }

    void enlarge_array(ArenaAllocator& arena)
    {
        size_t buckets_capacity = this->buckets_capacity_;
        BucketRelPtr* new_buckets = arena.template allocate_untagged_array<BucketRelPtr>((1ull << buckets_capacity) * 2);

        this->copy_buckets(arena, buckets_.get(), buckets_capacity, new_buckets, buckets_capacity + 1);
        this->buckets_ = new_buckets;
        this->buckets_capacity_ = buckets_capacity + 1;
    }


    void shrink_array(ArenaAllocator& arena)
    {
        if (this->size_)
        {
            size_t buckets_capacity = this->buckets_capacity_;

            BucketRelPtr* new_buckets = arena.template allocate_untagged_array<BucketRelPtr>(
                        (1ull << (buckets_capacity - 1)));

            copy_buckets(arena, buckets_.get(), buckets_capacity, new_buckets, buckets_capacity - 1);

            this->buckets_ = new_buckets;
            this->buckets_capacity_ = buckets_capacity - 1;
        }
        else {
            this->buckets_ = nullptr;
            this->buckets_capacity_ = 0;
        }
    }

    void copy_buckets(
            ArenaAllocator& arena,
            BucketRelPtr* src_buckets,
            size_t src_buckets_capacity,
            BucketRelPtr* dst_buckets,
            size_t dst_buckets_capacity
    )
    {
        for (size_t c = 0; c < 1ull << src_buckets_capacity; c++)
        {            
            if (src_buckets[c].is_not_null())
            {
                Bucket* bucket = src_buckets[c].get();
                for (size_t d = 0; d < bucket->size; d++)
                {
                    KeyHolder& key = bucket->keys()[d];
                    ValueHolder& value = bucket->values()[d];

                    bool replace{};
                    this->insert_into_array(
                                arena, dst_buckets, dst_buckets_capacity,
                                detail::ElementHolderHelper<KeyHolder>::resolve(key),
                                detail::ElementHolderHelper<ValueHolder>::resolve(value),
                                replace);
                }
            }
        }
    }

    uint64_t to_bucket(uint64_t hash) const {
        return to_bucket(buckets_capacity_, hash);
    }

    uint64_t to_bucket(size_t capacity, uint64_t hash) const {
        return hash & ((1ull << capacity) - 1);
    }

    Bucket* resize_bucket(ArenaAllocator& arena, Bucket* bucket, uint32_t new_capacity)
    {
        if (MMA_LIKELY(new_capacity))
        {
            Bucket* new_bucket = arena.allocate_object_untagged<Bucket>(new_capacity);

            new_bucket->size = bucket->size;

            KeyHolder* keys = bucket->keys();
            ValueHolder* values = bucket->values();

            KeyHolder* new_keys = new_bucket->keys();
            ValueHolder* new_values = new_bucket->values();

            for (size_t c = 0; c < bucket->size; c++) {
                detail::CopyHelper<KeyHolder>::copy(new_keys[c], keys[c]);
                detail::CopyHelper<ValueHolder>::copy(new_values[c], values[c]);
            }

            return new_bucket;
        }
        else {
            return nullptr;
        }
    }


    static Bucket* create_bucket(ArenaAllocator& arena) {
        return arena.allocate_object_untagged<Bucket>(1);
    }

    static constexpr uint64_t values_alignment_mask() noexcept
    {
        size_t align = alignof(Value);
        return align - 1;
    }

    void insert_into_array(
            ArenaAllocator& arena,
            BucketRelPtr* buckets,
            size_t capacity,
            const Key& key,
            const Value& value,
            bool& replace
    )
    {
        Hash<Key> hh;
        uint64_t bucket_idx = this->to_bucket(capacity, hh(key));

        if (buckets[bucket_idx].is_not_null())
        {
            Bucket* bucket = buckets[bucket_idx].get();
            KeyHolder* keys = bucket->keys();

            uint32_t idx;
            if (replace)
            {
                idx = bucket->find(key);
                if (idx < bucket->size) {
                    bucket->values()[idx] = value;
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
            }
            else {
                Bucket* new_bucket = this->resize_bucket(arena, bucket, bucket->capacity * 2);
                buckets[bucket_idx] = new_bucket;

                new_bucket->size++;
                KeyHolder* new_keys = new_bucket->keys();
                ValueHolder* new_values = new_bucket->values();

                new_keys[idx] = key;
                new_values[idx] = value;
            }
        }
        else {
            replace = false;

            Bucket* new_bucket = this->create_bucket(arena);
            new_bucket->size = 1;

            KeyHolder* new_keys = new_bucket->keys();
            ValueHolder* new_values = new_bucket->values();

            *new_keys = key;
            *new_values = value;

            buckets[bucket_idx] = new_bucket;
        }
    }

    Bucket* get_bucket(size_t bucket_idx) {
        return buckets_.get()[bucket_idx].get();
    }

    const Bucket* get_bucket(size_t bucket_idx) const {
        return buckets_.get()[bucket_idx].get();
    }

    bool is_bucket_null(size_t bucket_idx) const noexcept {
        return buckets_.get()[bucket_idx].is_null();
    }

public:
    void dump_state() const
    {
        size_t array_size = 1ull << buckets_capacity_;
        println("Map[{}, {}]{{", size_, array_size);

        uint64_t height{};
        uint64_t non_null{};

        if (buckets_.is_not_null())
        {
            for (size_t c = 0; c < array_size; c++)
            {                
                if (!is_bucket_null(c))
                {
                    const Bucket* bucket = get_bucket(c);
                    print("\t{}: <",c);

                    auto keys   = bucket->keys();
                    auto values = bucket->values();

                    height += bucket->size;
                    non_null++;

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

        println("}}({})", non_null > 0 ? (double)height / non_null : 0);
    }


    Map* deep_copy_to(
            ArenaAllocator& dst,
            ShortTypeCode tag,
            void* owner_view,
            ViewPtrHolder* ptr_holder,
            DeepCopyDeduplicator& dedup) const
    {
        Map* existing = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)existing)) {
            return existing;
        }
        else {
            auto map = dst.get_resolver_for(dst.template allocate_tagged_object<Map>(tag));
            dedup.map(dst, this, map.get(dst));

            map.get(dst)->size_ = size_;
            map.get(dst)->buckets_capacity_ = buckets_capacity_;

            if (buckets_.is_not_null())
            {
                auto buckets = dst.get_resolver_for(dst.template allocate_untagged_array<BucketRelPtr>(
                                                        1ull << buckets_capacity_));

                map.get(dst)->buckets_ = buckets.get(dst);
                const BucketRelPtr* src_buckets = buckets_.get();

                for (size_t c = 0; c < 1ull << buckets_capacity_; c++)
                {
                    if (src_buckets[c].is_not_null())
                    {
                        const Bucket* src_bucket = src_buckets[c].get();
                        auto dst_bucket = dst.get_resolver_for(
                            dst.allocate_object_untagged<Bucket>(src_bucket->capacity)
                        );

                        dst_bucket.get(dst)->size = src_bucket->size;
                        buckets.get(dst)[c] = dst_bucket.get(dst);

                        auto keys   = dst.get_resolver_for(dst_bucket.get(dst)->keys());
                        auto values = dst.get_resolver_for(dst_bucket.get(dst)->values());

                        memoria::detail::DeepCopyHelper<KeyHolder>::deep_copy_to(dst, keys, src_bucket->keys(), src_bucket->size, owner_view, ptr_holder, dedup);
                        memoria::detail::DeepCopyHelper<ValueHolder>::deep_copy_to(dst, values, src_bucket->values(), src_bucket->size, owner_view, ptr_holder, dedup);
                    }                    
                }
            }

            return map.get(dst);
        }
    }
};

}}
