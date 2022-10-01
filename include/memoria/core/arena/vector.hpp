
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

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/tools/span.hpp>



namespace memoria {
namespace arena {

namespace detail {

template <typename T>
struct CopyHelper {
    static void copy(T& dst, T& src) {
        dst = src;
    }
};

template <typename T>
struct CopyHelper<RelativePtr<T>> {
    static void copy(RelativePtr<T>& dst, const RelativePtr<T>& src) {
        dst = src.get();
    }
};

}

template <typename T>
class Vector {
    uint64_t size_;
    uint64_t capacity_;
    RelativePtr<T> data_;
public:
    Vector() noexcept :
        size_(), capacity_()
    {}

    uint64_t size() const {return size_;}

    uint64_t capacity() const {
        return capacity_;
    }

    bool has_extra_space() const {
        return capacity() > size_;
    }

    const T& get(size_t idx) const {
        return data_.get()[idx];
    }

    T& get(size_t idx) {
        return data_.get()[idx];
    }

    void set(size_t idx, const T& value)
    {
        detail::CopyHelper<T>::copy(data_.get()[idx], value);
    }

    void push_back(ArenaAllocator& arena, const T& value)
    {
        size_t cc = capacity();
        if (size_ >= cc) {
            enlarge(arena, cc + 1);
        }

        set(size_, value);
        size_++;
    }

    void push_back(ArenaAllocator& arena, Span<const T> span)
    {
        size_t cc = capacity();
        if (size_ + span.size() > cc) {
            enlarge(arena, span.size() + size_ - cc);
        }

        T* data = data_.get();

        for (size_t c = 0; c < span.size(); c++) {
            data[c] = span[c];
        }

        size_ += span.size();
    }

    Span<const T> span() const {
        return Span<const T>(data_.get(), size_);
    }

    Span<T> span() {
        return Span<T>(data_.get(), size_);
    }


    void enlarge(ArenaAllocator& arena, size_t target_size)
    {
        size_t cc = capacity_ > 0 ? capacity_ : 2;
        while (cc < target_size) {
            cc *= 2;
        }

        auto new_data = arena.template allocate_untagged_array<T>(cc);

        T* data = data_.get();

        for (size_t c = 0; c < size_; c++) {
            detail::CopyHelper<T>::copy(new_data[c], data[c]);
        }

        data_ = new_data;
        capacity_ = cc;
    }
};

using GenericVector = Vector<RelativePtr<void>>;

}}
