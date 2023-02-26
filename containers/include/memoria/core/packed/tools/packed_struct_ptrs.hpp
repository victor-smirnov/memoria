
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

#include <memoria/core/memory/ptr_cast.hpp>

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
        PackedAllocatable* object = ptr_cast<PackedAllocatable>(ptr);
        PackedAllocator*   alloc  = object->allocator();

        free_system(alloc);
    }

    template <class Tp>
    struct SharedPtrAllocator {
      typedef Tp value_type;

      size_t block_size_;
      size_t struct_size_;

      SharedPtrAllocator(size_t block_size, size_t struct_size):
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
              MMA_THROW(OOMException());
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
    PkdStructSPtr<T>
>
MakeSharedPackedStructByBlock(size_t block_size, Args&&... args)
{
    PkdStructSPtr<T> ptr = std::allocate_shared<T>(SharedPtrAllocator<T>(block_size, sizeof(T)));

    ptr->allocatable().setTopLevelAllocator();
    ptr->init_bs(block_size, std::forward<Args>(args)...);
    ptr->set_block_size(block_size);

    return ptr;
}


template <typename T, typename... Args>
std::enable_if_t<
    !std::is_base_of<PackedAllocator, T>::value,
    PkdStructSPtr<T>
>
MakeSharedPackedStructByBlock(size_t block_size, Args&&... args)
{
    size_t allocator_block_size = PackedAllocator::block_size(block_size, 1);

    PackedAllocator* alloc = allocate_system_zeroed<PackedAllocator>(allocator_block_size).release();

    if (alloc)
    {
        alloc->allocatable().setTopLevelAllocator();
        alloc->init(allocator_block_size, 1);

        T* ptr = alloc->template allocate_space<T>(0, block_size);

        ptr->init(std::forward<Args>(args)...);

        return PkdStructSPtr<T>(ptr, free_packed_allocatable);
    }
    else {
        MMA_THROW(OOMException());
    }
}

template <typename PkdStruct>
class PkdStructHolder {

protected:
    PkdStructUPtr<PackedAllocator> allocator_;
    size_t block_size_;

    static constexpr size_t STRUCT_IDX = 0;
    using PkdSO   = typename PkdStruct::SparseObject;
    using ExtData = typename PkdStruct::ExtData;

    ExtData ext_data_;


public:
    PkdStructHolder(size_t block_size):
        block_size_(block_size), ext_data_()
    {
        size_t allocator_block_size = PackedAllocator::block_size(block_size, 1);
        PackedAllocator* alloc = allocate_system_zeroed<PackedAllocator>(allocator_block_size).release();
        if (alloc) {
            allocator_ = PkdStructUPtr<PackedAllocator>(alloc, memoria::free_system);

            alloc->init(allocator_block_size, 1);
            alloc->allocate_empty<PkdStruct>(STRUCT_IDX);
        }
        else {
            MEMORIA_MAKE_PACKED_OOM_ERROR().do_throw();
        }
    }

    virtual ~PkdStructHolder() noexcept = default;

    size_t memory_block_size() const {return block_size_;}
    size_t allocated_block_size() const {return allocator_->block_size();}


    PkdSO get_so() {
        return PkdSO(&ext_data_, get_ptr());
    }

    const PkdSO get_so() const {
        return PkdSO(const_cast<ExtData*>(&ext_data_), const_cast<PkdStruct*>(get_ptr()));
    }

    static std::shared_ptr<PkdStructHolder> make_empty(size_t mem_block_size) {
        return std::make_shared<PkdStructHolder>(mem_block_size);
    }

    static std::unique_ptr<PkdStructHolder> make_empty_unique(size_t mem_block_size) {
        return std::make_unique<PkdStructHolder>(mem_block_size);
    }

    PkdStruct* get_ptr() {return allocator_->template get<PkdStruct>(STRUCT_IDX);}
    const PkdStruct* get_ptr() const {return allocator_->template get<PkdStruct>(STRUCT_IDX);}
};

}
