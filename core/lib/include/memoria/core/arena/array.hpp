
// Copyright 2022 Victor Smirnov
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
#include <memoria/core/tools/span.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/tools/result.hpp>

#include <memoria/core/reflection/reflection.hpp>
#include <memoria/core/hermes/serialization.hpp>


namespace memoria {
namespace arena {

// Note, must be sure that Array size is at least 1

template <typename T>
class Array {
protected:
    uint64_t size_;
    uint64_t capacity_;
    T array_[1];
public:
    static constexpr bool UseObjectSize = true;

    Array():
        size_(),
        capacity_(1)
    {}

    Array(size_t capacity) noexcept:
        size_(0), capacity_(capacity)
    {}

    uint64_t size() const {
        return size_;
    }

    uint64_t capacity() const {
        return capacity_;
    }

    Array* push_back(ArenaAllocator& arena, ShortTypeCode tag, const T& value)
    {
        if (MMA_LIKELY(size_ < capacity_)) {
            data()[size_] = value;
            size_++;
            return this;
        }
        else {
            uint64_t new_capacity = capacity_ * 2;
            Array* new_array = arena.allocate_tagged_object<Array>(tag, new_capacity);
            new_array->size_ = size_;

            T* data     = this->data();
            T* new_data = new_array->data();
            for (uint64_t c = 0; c < size_; c++) {
                new_data[c] = data[c];
            }

            return new_array->push_back(arena, tag, value);
        }
    }

    Array* remove(ArenaAllocator& arena, ShortTypeCode tag, size_t idx)
    {
        if (idx < size_)
        {
            auto data = this->data();
            if ((size_ - 1) >= capacity_ / 2)
            {
                for (size_t c = idx + 1; c < size_; c++) {
                    data[c - 1] = data[c];
                }
                --size_;

                return this;
            }
            else {
                uint64_t new_capacity = capacity_ / 2;
                Array* new_array = arena.allocate_tagged_object<Array>(tag, new_capacity);
                auto new_data = new_array->data();

                for (uint64_t c = 0; c < idx; c++) {
                    new_data[c] = data[c];
                }

                for (uint64_t c = idx + 1; c < size_; c++) {
                    new_data[c - 1] = data[c];
                }

                new_array->size_ = size_ - 1;

                return new_array;
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in arena::Array::remove {} {}", idx, size_).do_throw();
        }
    }

    void set(uint64_t idx, const T& value)
    {
        if (MMA_LIKELY(idx < size_)) {
            data()[idx] = value;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in arena::Array::set {} {}", idx, size_).do_throw();
        }
    }

    const T& get(uint64_t idx) const
    {
        if (MMA_LIKELY(idx < size_)) {
            return data()[idx];
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in arena::Array::get {} {}", idx, size_).do_throw();
        }
    }

    Span<T> span_mut() {
        return Span<T>(data_mut(), size_);
    }

    Span<const T> span() const {
        return Span<const T>(data(), size_);
    }

    T* data_mut() {
        return ptr_cast<T>(array_);
    }

    T* data() {
        return ptr_cast<T>(array_);
    }

    const T* data() const {
        return ptr_cast<const T>(array_);
    }

    static size_t object_size(size_t capacity = 1) {
        if (MMA_UNLIKELY(capacity == 0)) {
            capacity = 1;
        }
        return capacity * sizeof(T) + sizeof(Array) - sizeof(T);
    }

    Array* deep_copy_to(
            ShortTypeCode tag,
            hermes::DeepCopyState& dedup) const
    {
        auto& dst = dedup.arena();

        Array* existing = dedup.resolve(dedup.arena(), this);
        if (MMA_LIKELY((bool)existing)) {
            return existing;
        }
        else {
            auto vv = dst.get_resolver_for(dst.template allocate_tagged_object<Array>(tag, size_));
            dedup.map(dst, this, vv.get(dst));

            vv.get(dst)->size_ = size_;
            vv.get(dst)->capacity_ = size_;

            auto data = dst.get_resolver_for(vv.get(dst)->data());

            const T* src_data = this->data();
            memoria::detail::DeepCopyHelper<T>::deep_copy_to(data, src_data, size_, dedup);

            return vv.get(dst);
        }
    }

    void check_typed_array(hermes::CheckStructureState& state) const
    {
        state.mark_as_processed(this);
        state.check_and_set_tagged(this, object_size(capacity_), MA_SRC);


        if (size_ > capacity_) {
            MEMORIA_MAKE_GENERIC_ERROR("Array<T> size/capacity check error: {} {}", size_, capacity_).do_throw();
        }
    }

    void check_object_array(hermes::CheckStructureState& state) const
    {
        state.mark_as_processed(this);
        state.check_and_set_tagged(this, object_size(capacity_), MA_SRC);

        if (size_ > capacity_) {
            MEMORIA_MAKE_GENERIC_ERROR("Array<Object> size/capacity check error: {} {}", size_, capacity_).do_throw();
        }

        for (size_t c = 0; c < size_; c++) {
            state.check_ptr(get(c), MA_SRC);
        }

        for (size_t c = size_; c < capacity_; c++)
        {
            if (get(c).is_pointer())
            {
                if (get(c).is_not_null()) {
                    MEMORIA_MAKE_GENERIC_ERROR("Array<Object> element {} must be null at {}", c, MA_SRC).do_throw();
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Array<Object> element {} must be pointer at {}", c, MA_SRC).do_throw();
            }
        }
    }
};


}}
