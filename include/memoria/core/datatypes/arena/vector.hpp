
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

#include <memoria/core/datatypes/arena/arena.hpp>
#include <memoria/core/datatypes/arena/array.hpp>

#include <memoria/core/tools/span.hpp>



namespace memoria {
namespace arena {

template <typename T>
class Vector {
    uint64_t size_;
    uint64_t capacity_;
    SegmentPtr<T> data_;
public:
    Vector() noexcept :
        size_()
    {}

    uint64_t size() const {return size_;}

    uint64_t capacity() const {
        return capacity_;
    }

    bool has_extra_space(const MemorySegment* sgm) const {
        return capacity() > size_;
    }

    const T& get(const MemorySegment* sgm, size_t idx) const {
        return *data_.read(sgm)->span()[idx];
    }

    void set(ArenaSegment* sgm, size_t idx, const T& value) {
        *data_.write(sgm)->span_mut()[idx] = value;
    }

    void push_back(ArenaSegment* sgm, const T& value)
    {
        size_t cc = capacity();
        if (size_ >= cc) {
            enlarge(sgm, cc + 1);
        }

        *data_.write(sgm, size_) = value;
        size_++;
    }

    void push_back(ArenaSegment* sgm, Span<const T> span)
    {
        size_t cc = capacity();
        if (size_ + span.size() > cc) {
            enlarge(sgm, span.size() + size_ - cc);
        }

        T* ptr = data_.write(sgm, size_);

        for (size_t c = 0; c < span.size(); c++) {
            ptr[c] = span[c];
        }

        size_ += span.size();
    }

    Span<const T> span(const MemorySegment* sgm) const {
        return Span<const T>(data_.read(sgm), size_);
    }

    Span<T> span(ArenaSegment* sgm) const {
        return Span<T>(data_.write(sgm), size_);
    }

protected:
    void enlarge(ArenaSegment* sgm, size_t target_size)
    {
        size_t cc = capacity_ > 0 ? capacity_ : 2;
        while (cc < target_size) {
            cc *= 2;
        }

        auto new_data = SegmentPtr<T>::from(sgm->allocate_space(cc * sizeof(T), alignof(T), 0));

        const T* ptr = data_.read(sgm);
        T* new_ptr = new_data.write(sgm);

        for (size_t c = 0; c < size_; c++) {
            new_ptr[c] = ptr[c];
        }

        data_ = new_data;
        capacity_ = cc;
    }
};




}}
