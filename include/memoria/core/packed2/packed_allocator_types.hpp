
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

class PackedAllocator;

class PackedAllocatable {
protected:
	Int allocator_offset_;

	Int& allocator_offset() {return allocator_offset_;}

public:

	static const Int AlignmentBlock = PackedAllocationAlignment;

	PackedAllocatable() {}

	const Int& allocator_offset() const {return allocator_offset_;}

	template <typename MyType, typename Base>
	friend class PackedAllocatorBase;

	friend class PackedAllocator;

	void setAllocatorOffset(const void* allocator)
	{
		const char* my_ptr = T2T<const char*>(this);
		const char* alc_ptr = T2T<const char*>(allocator);
		size_t diff = T2T<size_t>(my_ptr - alc_ptr);
		allocator_offset() = diff;
	}

	PackedAllocator* allocator()
	{
		if (allocator_offset() > 0)
		{
			UByte* my_ptr = T2T<UByte*>(this);
			return T2T<PackedAllocator*>(my_ptr - allocator_offset());
		}
		else {
			return nullptr;
		}
	}

	const PackedAllocator* allocator() const
	{
		if (allocator_offset() > 0)
		{
			const UByte* my_ptr = T2T<const UByte*>(this);
			return T2T<const PackedAllocator*>(my_ptr - allocator_offset());
		}
		else {
			return nullptr;
		}
	}

	static Int roundBytesToAlignmentBlocks(Int value)
	{
		return (value / AlignmentBlock + (value % AlignmentBlock ? 1 : 0)) * AlignmentBlock;
	}

	static Int roundBitsToAlignmentBlocks(Int bits)
	{
		return roundBytesToAlignmentBlocks(roundBitToBytes(bits));
	}

	static Int roundBitToBytes(Int bits)
	{
		return bits / 8 + (bits % 8 > 0);
	}
};

template <Int Alignment = PackedAllocationAlignment>
struct PackedAllocatorTypes {
	static const Int AllocationAlignment = Alignment;
};


struct AllocationBlock {
	Int size_;
	Int offset_;
	UByte* ptr_;
	bool success_;

	AllocationBlock(Int size, Int offset, UByte* ptr): size_(size), offset_(offset), ptr_(ptr), success_(true) {}
	AllocationBlock(Int size): size_(size), offset_(0), ptr_(0), success_(false) {}

	Int size() const 	{return size_;}
	Int offset() const 	{return offset_;}
	UByte* ptr() const 	{return ptr_;}
	bool success() const {return success_;}

	operator bool() const {
		return success_;
	}

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
	const UByte* ptr_;

	AllocationBlockConst(Int size, Int offset, const UByte* ptr): size_(size), offset_(offset), ptr_(ptr) {}

	Int size() const 	{return size_;}
	Int offset() const 	{return offset_;}
	const UByte* ptr() const 	{return ptr_;}

	operator bool() const {return true;}

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
