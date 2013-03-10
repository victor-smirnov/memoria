
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_ALLOCATOR_TYPES_HPP_
#define MEMORIA_CORE_PACKED_ALLOCATOR_TYPES_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

namespace memoria {

template <typename MyType, typename Base> class PackedAllocatorBase;

class PackedAllocatable {
protected:
	Int allocator_offset_;

	Int& allocator_offset() {return allocator_offset_;}

public:

	PackedAllocatable() {}

	const Int& allocator_offset() const {return allocator_offset_;}

	template <typename MyType, typename Base>
	friend class PackedAllocatorBase;

	void setAllocatorOffset(const void* allocator)
	{
		const char* my_ptr = T2T<const char*>(this);
		const char* alc_ptr = T2T<const char*>(allocator);
		size_t diff = T2T<size_t>(my_ptr - alc_ptr);
		allocator_offset() = diff;
	}
};

template <Int Alignment = PackedAllocationAlignment>
struct PackedAllocatorTypes {
	static const Int AllocationAlignment = Alignment;
};


struct AllocationBlock {
	Int size_;
	Int offset_;
	char* ptr_;

	AllocationBlock(Int size, Int offset, char* ptr): size_(size), offset_(offset), ptr_(ptr) {}

	Int size() const 	{return size_;}
	Int offset() const 	{return offset_;}
	char* ptr() const 	{return ptr_;}

	template <typename T>
	const T* cast() const {
		return T2T<const T*>(ptr_);
	}

	template <typename T>
	T* cast() {
		return T2T<T*>(ptr_);
	}
};


struct AllocationBlockConst {
	Int size_;
	Int offset_;
	const char* ptr_;

	AllocationBlockConst(Int size, Int offset, const char* ptr): size_(size), offset_(offset), ptr_(ptr) {}

	Int size() const 	{return size_;}
	Int offset() const 	{return offset_;}
	const char* ptr() const 	{return ptr_;}

	template <typename T>
	const T* cast() const {
		return T2T<const T*>(ptr_);
	}
};

struct EmptyAllocator {
	Int enlargeBlock(void*, Int size) {return 0;}

	static Int roundBytesToAlignmentBlocks(int size) {return size;}
};

}


#endif
