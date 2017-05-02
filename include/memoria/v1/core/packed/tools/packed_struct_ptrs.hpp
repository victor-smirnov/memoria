
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>

#include <memory>
#include <type_traits>
#include <malloc.h>

namespace memoria {
namespace v1 {

template <typename PackedStruct>
using PkdStructUPtr = std::unique_ptr<PackedStruct, std::function<void (void*)>>;

template <typename PackedStruct>
using PkdStructSPtr = std::shared_ptr<PackedStruct>;

template <typename PackedStruct>
using PkdStructWPtr = std::weak_ptr<PackedStruct>;

namespace {

    void free_packed_allocatable(void* ptr)
    {
        PackedAllocatable* object = T2T<PackedAllocatable*>(ptr);
        PackedAllocator*   alloc  = object->allocator();

        ::free(alloc);
    }

    template <class Tp>
    struct SharedPtrAllocator {
      typedef Tp value_type;

      int32_t block_size_;
      int32_t struct_size_;

      SharedPtrAllocator(int32_t block_size, int32_t struct_size):
          block_size_(block_size), struct_size_(struct_size)
      {}

      template <class T>
      SharedPtrAllocator(const SharedPtrAllocator<T>& other) {
          block_size_   = other.block_size_;
          struct_size_  = other.struct_size_;
      }

      Tp* allocate(std::size_t n)
      {
          Tp* mem = T2T<Tp*>(malloc((sizeof(Tp) - struct_size_) + block_size_));

          if (mem) {
              return mem;
          }
          else {
              throw OOMException(MA_SRC);
          }
      }

      void deallocate(Tp* p, std::size_t n) {
          ::free(p);
      }

      template <typename Args>
      void construct(Tp* xptr, Args&& args) {}

      void destroy(Tp* xptr) {}
    };

    template <class T, class U>
    bool operator==(const SharedPtrAllocator<T>&, const SharedPtrAllocator<U>&) {
        return true;
    }

    template <class T, class U>
    bool operator!=(const SharedPtrAllocator<T>&, const SharedPtrAllocator<U>&) {
        return false;
    }
}


template <typename T, typename... Args>
std::enable_if_t<
    std::is_base_of<PackedAllocator, T>::value,
    PkdStructUPtr<T>
>
MakeUniquePackedStructByBlock(int32_t block_size, Args&&... args)
{
    MEMORIA_V1_ASSERT(block_size, >=, sizeof(T::empty_size()));

    void* block = malloc(block_size);
    memset(block, 0, block_size);

    if (block)
    {
        T* ptr = T2T<T*>(block);

        ptr->setTopLevelAllocator();
        ptr->init(std::forward<Args>(args)...);
        ptr->set_block_size(block_size);

        return PkdStructUPtr<T>(ptr, ::free);
    }
    else {
        throw OOMException(MA_SRC);
    }
}




template <typename T, typename... Args>
std::enable_if_t<
    !std::is_base_of<PackedAllocator, T>::value,
    PkdStructUPtr<T>
>
MakeUniquePackedStructByBlock(int32_t block_size, Args&&... args)
{
    int32_t allocator_block_size = PackedAllocator::block_size(block_size, 1);

    void* block = malloc(allocator_block_size);
    memset(block, 0, allocator_block_size);

    if (block)
    {
        PackedAllocator* alloc = T2T<PackedAllocator*>(block);

        alloc->setTopLevelAllocator();
        alloc->init(allocator_block_size, 1);

        T* ptr = alloc->template allocateSpace<T>(0, block_size);

        ptr->init(std::forward<Args>(args)...);

        return PkdStructUPtr<T>(ptr, free_packed_allocatable);
    }
    else {
        throw OOMException(MA_SRC);
    }
}


template <typename T, typename... Args>
std::enable_if_t<
    std::is_base_of<PackedAllocator, T>::value,
    PkdStructSPtr<T>
>
MakeSharedPackedStructByBlock(int32_t block_size, Args&&... args)
{
    PkdStructSPtr<T> ptr = std::allocate_shared<T>(SharedPtrAllocator<T>(block_size, sizeof(T)));

    ptr->setTopLevelAllocator();
    ptr->init(block_size, std::forward<Args>(args)...);
    ptr->set_block_size(block_size);

    return ptr;
}


template <typename T, typename... Args>
std::enable_if_t<
    !std::is_base_of<PackedAllocator, T>::value,
    PkdStructSPtr<T>
>
MakeSharedPackedStructByBlock(int32_t block_size, Args&&... args)
{
    int32_t allocator_block_size = PackedAllocator::block_size(block_size, 1);

    void* block = malloc(allocator_block_size);
    memset(block, 0, allocator_block_size);

    if (block)
    {
        PackedAllocator* alloc = T2T<PackedAllocator*>(block);

        alloc->setTopLevelAllocator();
        alloc->init(allocator_block_size, 1);

        T* ptr = alloc->template allocateSpace<T>(0, block_size);

        ptr->init(std::forward<Args>(args)...);

        return PkdStructSPtr<T>(ptr, free_packed_allocatable);
    }
    else {
        throw OOMException(MA_SRC);
    }
}



}}
