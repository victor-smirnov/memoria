
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
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/span.hpp>

#include <memoria/v1/core/linked/common/arena.hpp>

#include <cstddef>

namespace memoria {
namespace v1 {

template <typename T>
class LinkedVector {
    static_assert(std::is_trivially_copyable<T>::value, "");

    uint32_t capacity_;
    uint32_t size_;
    T data_[1];
public:
    static constexpr bool UseObjectSize = true;

    LinkedVector(uint32_t capacity, uint32_t size = 0) noexcept :
        capacity_(capacity), size_(size)
    {
        for (size_t c = 0; c < capacity_; c++) {
            data_[c] = T{};
        }
    }

    template <typename Ptr, typename Arena>
    LinkedVector(uint32_t capacity, LinkedPtrResolver<Ptr, Arena> other_r) noexcept :
        capacity_(capacity)
    {
        const LinkedVector* other = other_r.get();

        this->size_ = other->size_;

        for (size_t c = 0; c < size_; c++) {
            data_[c] = other->data_[c];
        }

        for (size_t c = size_; c < capacity_; c++) {
            data_[c] = T{};
        }
    }

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return capacity_;
    }

    T& access(size_t idx) noexcept {
        return data_[idx];
    }

    const T& access(size_t idx) const noexcept {
        return data_[idx];
    }

    bool has_space_for(size_t size) const {
        return (size_t)size_ + size <= (size_t)capacity_;
    }

    Span<T> span() noexcept {
        return Span<T>(data_, size_);
    }

    Span<const T> span() const noexcept {
        return Span<const T>(data_, size_);
    }

    bool push_back(const T& value) noexcept
    {
        if (MMA1_LIKELY(size_ + 1 <= capacity_))
        {
            data_[size_++] = value;
            return true;
        }

        return false;
    }

    size_t free_slots() const {
        return capacity_ - size_;
    }

    size_t push_back_with_offset(const T& value) noexcept
    {
        size_t idx = size_++;
        data_[idx] = value;
        return offset_of(idx);
    }

    void remove(size_t idx, size_t size) noexcept
    {
        for (size_t c = idx + size; c < size_; c++)
        {
            data_[c - size] = data_[c];
        }

        for (size_t c = size_ - size; c < size_; c++)
        {
            data_[c] = T{};
        }

        size_ -= size;
    }

    bool insert(size_t idx, const T& value) noexcept
    {
        if (size_ + 1 <= capacity_)
        {
            size_ += 1;
            shift_right(idx, 1);
            data_[idx] = value;
            return true;
        }

        return false;
    }

    bool insert(size_t idx, Span<const T> data) noexcept
    {
        if ((size_t)size_ + data.size() <= (size_t)capacity_)
        {
            size_ += data.size();
            shift_right(idx, data.size());

            for (size_t c = idx; c < data.size(); c++) {
                data_[c] = data[c - idx];
            }

            return true;
        }

        return false;
    }

    static size_t object_size(size_t capacity, size_t size = 0) noexcept {
        return sizeof(LinkedVector) - sizeof(T) + sizeof(T) * capacity;
    }

    template <typename Ptr, typename Arena>
    static size_t object_size(size_t capacity, LinkedPtrResolver<Ptr, Arena>) noexcept {
        return object_size(capacity);
    }

    void copy_to(LinkedVector& dst) const
    {
        dst.size_ = size_;
        for (size_t c = 0; c < size_; c++) {
            dst.data_[c] = data_[c];
        }
    }

    void clear()
    {
        size_ = 0;
        for (size_t c = 0; c < capacity_; c++) {
            data_[c] = T{};
        }
    }

    size_t offset_of(size_t idx) const {
        return offsetof(LinkedVector, data_) + idx * sizeof(T);
    }

private:
    void shift_right(size_t idx, size_t size) noexcept
    {
        for (size_t c = size_ - size; c >= idx; c--) {
            data_[c + size] = data_[c];
        }
    }
};

}}
