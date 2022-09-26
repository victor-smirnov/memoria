
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
#include <memoria/core/tools/span.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/tools/result.hpp>

namespace memoria {
namespace arena {

// Note, must be sure that Array size is at least 1

template <typename T>
class Array {
protected:
    uint64_t size_;
    T array_[1];
public:
    Array() {}

    Array(size_t size) noexcept:
        size_(size)
    {}

    uint64_t size() const {
        return size_;
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
        return ptr_cast<const T*>(array_);
    }

    static size_t object_size(size_t size) {
        if (size == 0) {
            MEMORIA_MAKE_GENERIC_ERROR("arena::Array size must be >= 1").do_throw();
        }
        return size * sizeof(T) + sizeof(Array) - sizeof(T);
    }
};

}}
