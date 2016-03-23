
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>

#include <malloc.h>
#include <memory>
#include <type_traits>


namespace memoria {
namespace v1 {

template <typename T>
using FreeUniquePtr = std::unique_ptr<T, decltype(free)*>;


template <typename T>
FreeUniquePtr<T> AllocateUnique(size_t block_size, const char* source = MA_RAW_SRC)
{
    T* ptr = T2T<T*>(malloc(block_size));

    if (ptr != nullptr)
    {
        return FreeUniquePtr<T>(ptr, free);
    }
    else {
        throw new OOMException(source);
    }
}


inline auto AllocateAllocator(size_t block_size, Int blocks = 1)
{
    auto ptr = AllocateUnique<PackedAllocator>(block_size);

    ptr->setTopLevelAllocator();
    ptr->init(block_size, blocks);

    return ptr;
}


}}