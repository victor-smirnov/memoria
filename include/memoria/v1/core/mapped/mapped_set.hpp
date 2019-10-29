
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/mapped/arena.hpp>
#include <memoria/v1/core/mapped/mapped_vector.hpp>
#include <memoria/v1/core/mapped/mapped_tools.hpp>
#include <memoria/v1/core/tools/optional.hpp>

#include <functional>
#include <deque>

namespace memoria {
namespace v1 {



template <
    typename Key,
    typename Arena,
    template <typename, typename> class Hash      = MappedHashFn,
    template <typename, typename> class KeyEqual  = MappedEqualToFn
>
class MappedSet {

    template <typename T>
    using PtrT = typename Arena::template PtrT<T>;


    using Bucket = MappedVector<Key>;
    using BucketPtr = PtrT<Bucket>;

    using Array = MappedVector<BucketPtr>;
    using ArrayPtr = PtrT<Array>;

public:
    struct State {
        uint32_t size_;
        ArrayPtr array_;
    };

private:
    Arena* arena_;
    PtrT<State> state_;

public:
    MappedSet(): arena_(), state_() {}

    MappedSet(Arena* arena, PtrT<State> state_ptr):
        arena_(arena), state_(state_ptr)
    {}

    auto ptr() const {
        return state_.get();
    }

    static MappedSet create(Arena* arena)
    {
        PtrT<State> ptr = arena->template allocate_space<State>(sizeof(State));
        arena->construct(ptr, State{});
        return MappedSet{arena, ptr.get()};
    }

    static MappedSet get(Arena* arena, PtrT<State> ptr) {
        return MappedSet{arena, ptr.get()};
    }

    void put(const Key& key)
    {
        ArrayPtr array = ensure_array_exists();
        put_to(array, key, nullptr);

        size_t size = state()->size_;

        if (size > 0)
        {
            Array* array = this->array();
            uint32_t capacity_threshold = (array->size() * 3) / 4;
            if (size > capacity_threshold) {
                rehash(array->size() * 2);
            }
        }

        state()->size_++;
    }

    template <typename KeyView>
    Optional<Key> get(const KeyView& key) const
    {
        const State* state = this->state();
        if (MMA1_UNLIKELY(state->size_ == 0)) {
            return Optional<Key>{};
        }

        const Array* array = state->array_.get(arena_);
        size_t slot  = array_slot(array, key);

        BucketPtr bucket = array->access(slot);
        if (MMA1_LIKELY(bucket))
        {
            Optional<size_t> idx = locate(bucket, key);
            if (idx) {
                return Optional<Key>(deref(bucket)->access(idx.get()));
            }
        }

        return Optional<Key>{};
    }

    template <typename Fn>
    void for_each(Fn&& fn) const
    {
        const Array* array = this->array();
        if(!arena_->is_null(array))
        {
            for_each(array, std::forward<Fn>(fn));
        }
    }

    template <typename KeyView>
    bool remove(const KeyView& key_view)
    {
        ArrayPtr array = state()->array_;
        if (array)
        {
            Array* parray = deref(array);
            size_t slot  = array_slot(parray, key_view);

            BucketPtr bucket = parray->access(slot);
            if (MMA1_LIKELY(bucket))
            {
                Optional<size_t> idx = locate(bucket, key_view);
                if (idx) {
                    Bucket* pbucket = deref(bucket);

                    pbucket->remove(idx.get(), 1);

                    if (pbucket->size() == 0) {
                        parray->access(slot) = BucketPtr{};
                    }

                    size_t size = --state()->size_;

                    if (size > 0)
                    {
                        uint32_t capacity_threshold = parray->size() / 4;
                        if (size <= capacity_threshold) {
                            rehash(parray->size() / 2);
                        }
                    }
                    else {
                        state()->array_ = ArrayPtr{};
                    }

                    return true;
                }
            }
        }

        return false;
    }

    size_t size() const {
        return state()->size_;
    }

    size_t array_size() const {
        return array()->size();
    }

    PtrT<State> deep_copy_to(Arena* dst, typename Arena::AddressMapping& visited_addresses) const
    {
        PtrT<State> foreign_state = allocate<State>(dst);

        ArrayPtr my_array = state()->array_;
        if (my_array)
        {
            Array* pmy_array = deref(my_array);
            auto span = pmy_array->span();

            ArrayPtr foreign_array = allocate<Array>(dst, pmy_array->capacity(), pmy_array->size());

            for (size_t c = 0; c < span.size(); c++)
            {
                BucketPtr my_bucket = span[c];
                if (my_bucket)
                {
                    const Bucket* pmy_bucket = deref(my_bucket);
                    BucketPtr foreign_bucket = allocate<Bucket>(dst, pmy_bucket->size(), pmy_bucket->size());

                    for (size_t d = 0; d < pmy_bucket->size(); d++)
                    {
                        const Key& entry = pmy_bucket->access(d);

                        Key foreign_key = mapped_::DeepCopyHelper<Key>::deep_copy(dst, arena_, entry, visited_addresses);

                        foreign_bucket.get(dst)->access(d) = foreign_key;
                    }

                    foreign_array.get(dst)->access(c) = foreign_bucket;
                }
            }

            State* fstate = foreign_state.get(dst);
            fstate->size_ = state()->size_;
            fstate->array_ = foreign_array;
        }
        else {
            State* fstate = foreign_state.get(dst);
            fstate->size_ = 0;
            fstate->array_ = ArrayPtr{};
        }

        return foreign_state;
    }

    void print_bucket_stat() const
    {
        ArrayPtr array = state()->array_;
        if (array)
        {
            size_t empty_{};
            size_t used_{};
            size_t total_bucket_length_{};
            size_t max_bucket_size_{};
            size_t total_bucket_data_length_{};

            const Array* parray = deref(array);
            for (BucketPtr bucket: parray->span())
            {
                if (bucket) {
                    used_++;
                    const Bucket* pbucket = deref(bucket);
                    if (pbucket->size() > max_bucket_size_) {
                        max_bucket_size_ = pbucket->size();
                    }

                    total_bucket_length_ += pbucket->size();
                    total_bucket_data_length_ += pbucket->object_size(pbucket->capacity());
                }
                else {
                    empty_++;
                }
            }

            std::cout << "Stat: "
                      << empty_ << " :: "
                      << used_ << " :: "
                      << total_bucket_length_ << " :: "
                      << max_bucket_size_ << " :: "
                      << total_bucket_data_length_
                      << std::endl;
        }
    }

private:

    template <typename TT>
    MappedPtrResolver<PtrT<TT>, Arena> resolver(PtrT<TT> ptr)
    {
        return MappedPtrResolver<PtrT<TT>, Arena>(ptr, arena_);
    }

    template <typename TT>
    TT* deref(PtrT<TT> ptr) {
        return ptr.get(arena_);
    }

    template <typename TT>
    TT* deref(PtrT<TT> ptr) const {
        return ptr.get(arena_);
    }

    State* state() {
        return deref(state_);
    }

    const State* state() const {
        return deref(state_);
    }

    const Array* array() const {
        return deref(state()->array_);
    }

    Array* array() {
        return deref(state()->array_);
    }

    template <typename KeyView>
    Optional<size_t> locate(BucketPtr bucket_ptr, const KeyView& key_view) const
    {
        KeyEqual<Key, Arena> equal_to_fn(arena_);

        size_t idx{};
        const Bucket* bucket = deref(bucket_ptr);
        for (auto& entry: bucket->span())
        {
            if (equal_to_fn(key_view, entry)) {
                return Optional<size_t>(idx);
            }

            idx++;
        }

        return Optional<size_t>();
    }

    void put_to(PtrT<Array> array_ptr, const Key& key, std::deque<BucketPtr>* buffer_cache)
    {
        Array* array = deref(array_ptr);
        size_t slot  = array_slot(array, key);

        BucketPtr bucket = array->access(slot);
        if (MMA1_UNLIKELY(!bucket))
        {
            BucketPtr new_bucket{};
            if (buffer_cache)
            {
                if (buffer_cache->size() > 0)
                {
                    new_bucket = buffer_cache->front();
                    buffer_cache->pop_front();
                }
                else {
                    new_bucket = create_bucket(1);
                }
            }
            else {
                new_bucket = create_bucket(1);
            }

            deref(array_ptr)->access(slot) = new_bucket;
            deref(new_bucket)->push_back(key);
        }
        else
        {
            auto idx = locate(bucket, key);

            if (idx) {
                // Do nothing
            }
            else if (MMA1_UNLIKELY(!deref(bucket)->push_back(key)))
            {
                BucketPtr new_bucket;

                if (buffer_cache)
                {
                    if (buffer_cache->size() > 0)
                    {
                        new_bucket = buffer_cache->front();
                        size_t new_capacity = deref(new_bucket)->capacity();
                        size_t existing_capacity = deref(bucket)->capacity();
                        if (new_capacity > existing_capacity)
                        {
                            deref(bucket)->copy_to(*deref(new_bucket));
                            buffer_cache->pop_front();
                        }
                        else {
                            new_bucket = enlarge_bucket(bucket);
                        }
                    }
                    else {
                        new_bucket = enlarge_bucket(bucket);
                    }
                }
                else {
                    new_bucket = enlarge_bucket(bucket);
                }


                deref(array_ptr)->access(slot) = new_bucket;
                deref(new_bucket)->push_back(key);
            }
        }
    }



    template <typename Fn>
    void for_each(const Array* array, Fn&& fn) const
    {
        for (const auto& value: array->span())
        {
            if (value)
            {
                const Bucket* bucket = deref(value);

                for (const auto& entry: bucket->span())
                {
                    fn(entry);
                }
            }
        }
    }



    void rehash(size_t new_size)
    {
        PtrT<Array> new_array = allocate<Array>(
            arena_, new_size, new_size
        );

        std::deque<BucketPtr>   buffer_cache;
        ArenaBuffer<Key>        tmp_bucket;
        ArenaBuffer<BucketPtr>  tmp_array;

        tmp_array.append_values(deref(state()->array_)->span());

        for (const auto& value: tmp_array.span())
        {
            if (value)
            {
                const Bucket* bucket = deref(value);

                tmp_bucket.clear();
                tmp_bucket.append_values(bucket->span());

                for (const auto& entry: tmp_bucket.span())
                {
                    put_to(new_array, entry, &buffer_cache);
                }

                deref(value)->clear();
                buffer_cache.push_back(value);
            }
        }

        state()->array_ = new_array;
    }

    BucketPtr create_bucket(size_t capacity = 1) const {
        return allocate<Bucket>(arena_, capacity);
    }

    BucketPtr enlarge_bucket(BucketPtr bucket) const
    {
        BucketPtr new_bucket = create_bucket(deref(bucket)->capacity() * 2);
        deref(bucket)->copy_to(*deref(new_bucket));
        return new_bucket;
    }


    ArrayPtr ensure_array_exists()
    {
        if (!state()->array_)
        {
            size_t capacity = 1;

            PtrT<Array> array = arena_->template allocate_space<Array>(Array::object_size(capacity));
            arena_->construct(array, capacity);

            state()->array_ = array;

            Array* array_ptr = deref(array);

            for (size_t c = 0; c < capacity; c++) {
                array_ptr->push_back(BucketPtr{});
            }

            return array;
        }

        return state()->array_;
    }

    template <typename KeyArg>
    size_t array_slot(const Array* array, const KeyArg& key) const
    {
        Hash<Key, Arena> hash_fn(arena_);

        auto hash_code = hash_fn(key);

        return hash_code & array_size_mask(array);
    }

    template <typename KeyArg>
    size_t array_slot(const KeyArg& key) const
    {
        return array_slot(array(), key);
    }

    size_t array_size_mask() const
    {
        return array_size_mask(array());
    }

    size_t array_size_mask(const Array* array) const
    {
        size_t capacity = array->size();
        return capacity - 1;
    }
};

}}
