
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

#include <memoria/core/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memory>
#include <type_traits>
#include <stdlib.h>

namespace memoria {

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

        free_system(alloc);
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
          Tp* mem = allocate_system<Tp>((sizeof(Tp) - struct_size_) + block_size_).release();

          if (mem) {
              return mem;
          }
          else {
              MMA1_THROW(OOMException());
          }
      }

      void deallocate(Tp* p, std::size_t n) {
          free_system(p);
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

    T* ptr = allocate_system_zeroed<T>(block_size).release();

    if (ptr)
    {
        ptr->setTopLevelAllocator();
        OOM_THROW_IF_FAILED(ptr->init(std::forward<Args>(args)...), MMA1_SRC);
        ptr->set_block_size(block_size);

        return PkdStructUPtr<T>(ptr, free_system);
    }
    else {
        MMA1_THROW(OOMException());
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

    PackedAllocator* alloc = allocate_system_zeroed<PackedAllocator>(allocator_block_size).release();

    if (alloc)
    {
        alloc->allocatable().setTopLevelAllocator();
        OOM_THROW_IF_FAILED(alloc->init(allocator_block_size, 1), MMA1_SRC);

        T* ptr = alloc->template allocateSpace<T>(0, block_size);

        OOM_THROW_IF_FAILED(ptr->init(std::forward<Args>(args)...), MMA1_SRC);

        return PkdStructUPtr<T>(ptr, free_packed_allocatable);
    }
    else {
        MMA1_THROW(OOMException());
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

    ptr->allocatable().setTopLevelAllocator();
    OOM_THROW_IF_FAILED(ptr->init_bs(block_size, std::forward<Args>(args)...), MMA1_SRC);
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

    PackedAllocator* alloc = allocate_system_zeroed<PackedAllocator>(allocator_block_size).release();

    if (alloc)
    {
        alloc->allocatable().setTopLevelAllocator();
        OOM_THROW_IF_FAILED(alloc->init(allocator_block_size, 1), MMA1_SRC);

        T* ptr = alloc->template allocateSpace<T>(0, block_size);

        OOM_THROW_IF_FAILED(ptr->init(std::forward<Args>(args)...), MMA1_SRC);

        return PkdStructSPtr<T>(ptr, free_packed_allocatable);
    }
    else {
        MMA1_THROW(OOMException());
    }
}



}
