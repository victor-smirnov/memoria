
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
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/tools/alloc.hpp>

#include <stdlib.h>
#include <memory>
#include <type_traits>


namespace memoria {
namespace v1 {


template <typename T> struct AllocTool;

template <>
struct AllocTool<PackedAllocator> {
    using Type = PackedAllocator;


    static FreeUniquePtr<Type> create(int32_t block_size, int32_t slots)
    {
        int32_t full_block_size = Type::block_size(block_size, slots);

        auto ptr = AllocateUnique<Type>(full_block_size, MA_RAW_SRC);

        ptr->setTopLevelAllocator();
        ptr->init(full_block_size, slots);

        return ptr;
    }
};


//template <typename T>
//std::unique_ptr<T, decltype(free)*> PkdMakeUniqueByMemorySize(int32_t block_size)
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
//  int32_t block_size = T::packed_block_size(std::forward<Args>(args)...) + 1000;
//  return PkdMakeUniqueByMemorySize<T>(block_size);
//}





}}
