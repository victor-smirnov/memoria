
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/type2type.hpp>

#include <memory>
#include <cstdlib>
#include <cstring>

namespace memoria {
namespace v1 {

template <typename T>
using UniquePtr = std::unique_ptr<T, void (*)(void*)>;

template <typename T>
UniquePtr<T> allocate_system(size_t size)
{
    void* ptr = ::malloc(size);

    T* tptr = T2T<T*>(ptr);

    return UniquePtr<T>(tptr, ::free);
}

template <typename T>
UniquePtr<T> reallocate_system(void* ptr, size_t size)
{
    return UniquePtr<T>(T2T<T*>(::realloc(ptr, size)), ::free);
}

template <typename T>
UniquePtr<T> allocate_system_zeroed(size_t size)
{
    void* ptr = ::malloc(size);
    std::memset(ptr, 0, size);
    return UniquePtr<T>(T2T<T*>(ptr), ::free);
}

static inline void free_system(void* ptr) noexcept
{
    ::free(ptr);
}

template <typename T>
UniquePtr<T> empty_unique_ptr() 
{
	return UniquePtr<T>(T2T<T*>(nullptr), ::free);
}

}}
