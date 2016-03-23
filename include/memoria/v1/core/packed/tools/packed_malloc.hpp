
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/tools/alloc.hpp>

#include <malloc.h>
#include <memory>
#include <type_traits>


namespace memoria {
namespace v1 {


template <typename T> struct AllocTool;

template <>
struct AllocTool<PackedAllocator> {
    using Type = PackedAllocator;


    static FreeUniquePtr<Type> create(Int block_size, Int slots)
    {
        Int full_block_size = Type::block_size(block_size, slots);

        auto ptr = AllocateUnique<Type>(full_block_size);

        ptr->setTopLevelAllocator();
        ptr->init(full_block_size, slots);

        return ptr;
    }
};


//template <typename T>
//std::unique_ptr<T, decltype(free)*> PkdMakeUniqueByMemorySize(Int block_size)
//{
//  static_assert(std::is_base_of<PackedAllocatable, T>::value, "Only PackedAllocatable objects may be created by this function");
//
//  T* ptr = T2T<T*>(malloc(block_size));
//
//  if (ptr != nullptr)
//  {
//      std::unique_ptr<T, decltype(free)*> uptr(ptr, free);
//
//      uptr->setTopLevelAllocator();
//      uptr->init(block_size);
//
//      return uptr;
//  }
//  else {
//      throw new OOMException(MA_SRC);
//  }
//}
//
//template <typename T, typename... Args>
//std::unique_ptr<T, decltype(free)*> PkdMakeUniqueByDataSize(Args&&... args)
//{
//  Int block_size = T::packed_block_size(std::forward<Args>(args)...) + 1000;
//  return PkdMakeUniqueByMemorySize<T>(block_size);
//}





}}