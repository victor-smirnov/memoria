
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

#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <stdlib.h>
#include <memory>
#include <type_traits>


namespace memoria {

template <typename T>
using FreeUniquePtr = UniquePtr<T>;


template <typename T>
FreeUniquePtr<T> AllocateUnique(size_t block_size)
{
    auto ptr = allocate_system<T>(block_size);

    if (ptr.get())
    {
        return ptr;
    }
    else {
        MMA1_THROW(OOMException());
    }
}


inline auto AllocateAllocator(size_t block_size, int32_t blocks = 1)
{
    auto ptr = AllocateUnique<PackedAllocator>(block_size);

    ptr->allocatable().setTopLevelAllocator();
    OOM_THROW_IF_FAILED(ptr->init(block_size, blocks), MMA1_SRC);

    return ptr;
}


}