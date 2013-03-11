
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_DYNAMIC_ALLOCATOR_HPP_
#define MEMORIA_CORE_PACKED_DYNAMIC_ALLOCATOR_HPP_

#include <memoria/core/packed2/packed_allocator_base.hpp>
#include <memoria/core/packed2/packed_fse_tree.hpp>

#include <ostream>
#include <string.h>

namespace memoria {

using namespace std;

template <typename MyType, typename Base_>
class PackedDynamicAllocatorBase: public PackedAllocatorBase<MyType, Base_> {

	typedef PackedAllocatorBase<MyType, Base_> 									Base;
	typedef PackedDynamicAllocatorBase<MyType, Base_>							ThisType;

protected:
	static const Int AllocationAlignment 	= 	PackedAllocationAlignment;

	Int block_size_;
	Int layout_block_size_;
	char buffer_[];

	typedef PackedFSETreeTypes<Int, Int, Int> 	Types;

public:
	typedef PackedFSETree<Types>				Layout;

protected:

	typedef typename Layout::Value				Value;

protected:

	PackedDynamicAllocatorBase() {}

public:

	char* data()
	{
		return buffer_ + layout_block_size_;
	}

	const char* data() const
	{
		return buffer_ + layout_block_size_;
	}

	Int data_size() const
	{
		return block_size() - sizeof(MyType) - layout_block_size_;
	}

	Layout& layout() 				{return *T2T<Layout*>(buffer_);}
	const Layout& layout() const 	{return *T2T<const Layout*>(buffer_);}

	Int block_size() const 			{return block_size_;}

	static Int block_size_by_elements(Int elements_number)
	{
		Int requested_layout_size	= Layout::block_size(elements_number);
		Int layout_block_size 		= Base::roundBytesToAlignmentBlocks(requested_layout_size);

		return sizeof(MyType) + layout_block_size;
	}

	void init(Int block_size, Int elements_number)
	{
		block_size_ 				= block_size;
		Int requested_layout_size	= Layout::block_size(elements_number);
		layout_block_size_ 			= Base::roundBytesToAlignmentBlocks(requested_layout_size);

		Layout& the_layout = layout();

		the_layout.initBlockSize(requested_layout_size);

		MEMORIA_ASSERT(the_layout.max_size(), >=, elements_number);
		MEMORIA_ASSERT(the_layout.block_size(), <=, layout_block_size_);

		the_layout.size() = elements_number;

		the_layout.reindex();
	}

	AllocationBlock allocate(Int idx, Int size)
	{
		Int offset 			= getOffset(idx);
		char* ptr			= data() + offset;

		Int allocation_size = Base::enlargeBlock(idx, size);

		return AllocationBlock(allocation_size, offset, ptr);
	}

	AllocationBlock describe(Int idx)
	{
		Int offset 	= getOffset(idx);
		Int size	= getSize(idx);
		char* ptr	= data() + offset;

		return AllocationBlock(size, offset, ptr);
	}

	AllocationBlockConst describe(Int idx) const
	{
		Int offset 	= getOffset(idx);
		Int size	= getSize(idx);
		const char* ptr	= data() + offset;

		return AllocationBlockConst(size, offset, ptr);
	}

	void free(Int idx)
	{
		AllocationBlock block = describe(idx);
		Base::shrinkBlock(block.ptr(), block.size());
	}

	void clearMemory(Int idx)
	{
		Int offset 	= getOffset(idx);
		Int size 	= getSize(idx);
		char* ptr	= data() + offset;

		memset(ptr, 0, size);
	}

	void clearMemoryBlock()
	{
		memset(data(), 0, data_size());
	}

	template <typename T>
	const T* get(Int idx) const
	{
		return T2T<const T*>(data() + getOffset(idx));
	}

	template <typename T>
	T* get(Int idx)
	{
		return T2T<T*>(data() + getOffset(idx));
	}

	Int getElementsNumber() const
	{
		return layout().size();
	}

	Int getSize(Int idx) const
	{
		return layout().value(idx);
	}

	Int getOffset(Int idx) const
	{
		GetFSEValuesSumFn<Layout> fn(layout());

		layout().walk_range(idx, fn);

		return fn.sum();
	}

	void updateElementSize(Int idx, Int delta)
	{
		layout().value(idx) += delta;
		layout().reindex();
	}

	MyType& me() 				{return *static_cast<MyType*>(this);}
	const MyType& me() const 	{return *static_cast<const MyType*>(this);}

	void dump(ostream& out = cout) const
	{
		layout().dump(out);
	}
};


template <typename Base = EmptyType>
class PackedDynamicAllocator: public PackedDynamicAllocatorBase<PackedDynamicAllocator<Base>, Base> {
public:
	PackedDynamicAllocator() {}
};



class PackedSingleElementAllocator: public PackedAllocatorBase<PackedSingleElementAllocator> {

	typedef PackedAllocatorBase<PackedSingleElementAllocator> 		Base;
	typedef PackedSingleElementAllocator							MyType;

	Int block_size_;
	Int allocated_;

	char buffer_[];

public:
	PackedSingleElementAllocator() {}

	Int block_size() const {
		return block_size_;
	}

	static Int block_size(Int memory_size)
	{
		return memory_size + sizeof(MyType);
	}

	void init(Int block_size)
	{
		block_size_ = block_size;
	}

	template <typename Allocatable>
	const Allocatable* get() const
	{
		return T2T<const Allocatable*>(buffer_ + getOffset(0));
	}

	template <typename Allocatable>
	Allocatable* get()
	{
		return T2T<Allocatable*>(buffer_ + getOffset(0));
	}

	Int getElementsNumber() const
	{
		return 1;
	}

	Int getSize(Int idx) const
	{
		return allocated_;
	}

	Int getOffset(Int idx) const
	{
		return 0;
	}

	void updateElementSize(Int idx, Int delta)
	{
		allocated_ += delta;
	}

	AllocationBlock allocate(Int size)
	{
		Int allocation_size = Base::enlargeBlock(0, size);
		return AllocationBlock(allocation_size, 0, buffer_);
	}
};


}


#endif
