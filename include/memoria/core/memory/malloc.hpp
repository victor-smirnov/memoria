
// Copyright 2018 Victor Smirnov
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

#include <memoria/core/memory/ptr_cast.hpp>

#include <memory>
#include <cstdlib>
#include <cstring>

namespace memoria {

namespace detail {
    template <typename T>
    struct AllocationSizeH: HasValue<size_t, sizeof(T)> {};

    template <>
    struct AllocationSizeH<void>: HasValue<size_t, 1> {};
}

template <typename T>
using UniquePtr = std::unique_ptr<T, void (*)(void*)>;

template <typename T>
UniquePtr<T> allocate_system(size_t size)
{
    size_t alc_size = size * detail::AllocationSizeH<T>::Value;
    void* ptr = ::malloc(alc_size);

    T* tptr = ptr_cast<T>(ptr);

    return UniquePtr<T>(tptr, ::free);
}

template <typename T>
UniquePtr<T> allocate_block_of_size(size_t size)
{
    void* ptr = ::malloc(size);
    T* tptr = ptr_cast<T>(ptr);

    return UniquePtr<T>(tptr, ::free);
}

template <typename T>
UniquePtr<T> uptr_from_raw(T* ptr) {
    return UniquePtr<T>{ptr, ::free};
}

template <typename T>
UniquePtr<T> reallocate_system(void* ptr, size_t size)
{
    return UniquePtr<T>(ptr_cast<T>(::realloc(ptr, size * detail::AllocationSizeH<T>::Value)), ::free);
}

template <typename T>
UniquePtr<T> allocate_system_zeroed(size_t size)
{
    void* ptr = ::malloc(size * detail::AllocationSizeH<T>::Value);
    std::memset(ptr, 0, size * detail::AllocationSizeH<T>::Value);

    return UniquePtr<T>(ptr_cast<T>(ptr), ::free);
}

static inline void free_system(void* ptr) noexcept
{
    ::free(ptr);
}

template <typename T>
UniquePtr<T> empty_unique_ptr() 
{
    return UniquePtr<T>(ptr_cast<T>(nullptr), ::free);
}

}
