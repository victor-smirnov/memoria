
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_ALLOCATOR_BASE_HPP_
#define MEMORIA_CORE_PACKED_ALLOCATOR_BASE_HPP_

#include <memoria/core/packed2/packed_allocator_types.hpp>

namespace memoria {

template <typename MyType, typename Base = EmptyType>
class PackedAllocatorBase: public Base {

	static const Int AlignmentBlock = PackedAllocationAlignment;

public:
	PackedAllocatorBase() {}

	static Int roundBytesToAlignmentBlocks(Int value)
	{
		return (value / AlignmentBlock + (value % AlignmentBlock ? 1 : 0)) * AlignmentBlock;
	}

	Int getComputeOffset(const void* element) const
	{
		const char* base_ptr = me().getOffsetBase();
		const char* elt_ptr = T2T<const char*>(element);

		size_t diff = T2T<size_t>(elt_ptr - base_ptr);

		return diff;
	}

	Int enlargeBlock(const void* element, Int new_size)
	{
		Int allocation_size = roundBytesToAlignmentBlocks(new_size);

		Int elt_offset = getComputeOffset(element);

		Int elements_number = me().getElementsNumber();

		Int start_idx = elements_number;

		Int current_size = -1;

		for (Int idx = 0; idx < elements_number; idx++)
		{
			Int offset = me().getOffset(idx);

			if (offset == elt_offset)
			{
				start_idx = idx;
				current_size = me().getSize(idx);
				break;
			}
		}

		if (current_size >= 0)
		{
			Int required = allocation_size - current_size;

			if (me().available() < required)
			{
				if (!me().enlargeAllocator(required))
				{
					return 0;
				}
			}
		}

		for (Int idx = elements_number - 1; idx > start_idx; idx--)
		{
			Int offset  = me().getOffset(idx);
			Int size 	= me().getSize(idx);

			me().moveElement(idx, offset, size, allocation_size);
		}

		if (start_idx < elements_number)
		{
			me().updateElementSize(start_idx, allocation_size - current_size);
		}

		return allocation_size;
	}

	Int enlargeBlock(Int element_idx, Int new_size)
	{
		Int allocation_size = roundBytesToAlignmentBlocks(new_size);

		Int elements_number = me().getElementsNumber();
		Int current_size 	= me().getSize(element_idx);

		for (Int idx = elements_number - 1; idx > element_idx; idx--)
		{
			Int offset  = me().getOffset(idx);
			Int size 	= me().getSize(idx);

			if (size > 0)
			{
				me().moveElement(idx, offset, size, allocation_size);
			}
		}

		if (element_idx < elements_number)
		{
			me().updateElementSize(element_idx, allocation_size - current_size);
		}

		return allocation_size;
	}


	Int shrinkBlock(const void* element, Int new_size)
	{
		Int allocation_size = roundBytesToAlignmentBlocks(new_size);

		Int elt_offset = getComputeOffset(element);

		Int elements_number = me().getElementsNumber();

		Int start_idx = elements_number;

		Int current_size;

		for (Int idx = 0; idx < elements_number; idx++)
		{
			Int offset = me().getOffset(idx);

			if (offset == elt_offset)
			{
				start_idx = idx;
				current_size = me().getSize(idx);
				break;
			}
		}

		for (Int idx = start_idx + 1; idx < elements_number; idx++)
		{
			Int offset  = me().getOffset(idx);
			Int size 	= me().getSize(idx);

			if (size > 0)
			{
				me().moveElement(idx, offset, size, allocation_size - current_size);
			}
		}

		if (start_idx < elements_number)
		{
			me().updateElementSize(start_idx, allocation_size);
		}

		return allocation_size;
	}

	void moveElement(Int element_idx, Int offset, Int size, Int delta)
	{
		Byte* ptr = T2T<Byte*>(this) + offset;

		CopyByteBuffer(ptr , ptr + delta, size);

		PackedAllocatable* element = T2T<PackedAllocatable*>(ptr + delta);

		element->allocator_offset() += delta;
	}

	bool enlargeAllocator(Int delta) {return false;};

	MyType& me() 				{return *static_cast<MyType*>(this);}
	const MyType& me() const 	{return *static_cast<const MyType*>(this);}
};

}


#endif
