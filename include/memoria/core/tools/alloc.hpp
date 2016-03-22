
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <malloc.h>
#include <memory>
#include <type_traits>


namespace memoria {

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


}
