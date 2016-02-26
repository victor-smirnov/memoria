
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_PACKED_STRUCT_PTRS_HPP_
#define MEMORIA_CORE_PACKED_STRUCT_PTRS_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memory>
#include <type_traits>
#include <malloc.h>

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

		::free(alloc);
	}

	template <class Tp>
	struct SharedPtrAllocator {
	  typedef Tp value_type;

	  Int block_size_;
	  Int struct_size_;

	  SharedPtrAllocator(Int block_size, Int struct_size):
		  block_size_(block_size), struct_size_(struct_size)
	  {}

	  template <class T>
	  SharedPtrAllocator(const SharedPtrAllocator<T>& other) {
		  block_size_ 	= other.block_size_;
		  struct_size_	= other.struct_size_;
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
MakeUniquePackedStructByBlock(Int block_size, Args&&... args)
{
	MEMORIA_ASSERT(block_size, >=, sizeof(T::empty_size()));

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
MakeUniquePackedStructByBlock(Int block_size, Args&&... args)
{
	Int allocator_block_size = PackedAllocator::block_size(block_size, 1);

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
MakeSharedPackedStructByBlock(Int block_size, Args&&... args)
{
	PkdStructSPtr<T> ptr = std::allocate_shared<T>(SharedPtrAllocator<T>(block_size, sizeof(T)));

	ptr->setTopLevelAllocator();
	ptr->init(std::forward<Args>(args)...);
	ptr->set_block_size(block_size);

	return ptr;
}


template <typename T, typename... Args>
std::enable_if_t<
	!std::is_base_of<PackedAllocator, T>::value,
	PkdStructSPtr<T>
>
MakeSharedPackedStructByBlock(Int block_size, Args&&... args)
{
	Int allocator_block_size = PackedAllocator::block_size(block_size, 1);

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



}


#endif