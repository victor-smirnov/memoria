
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_ALLOCATOR_HPP_
#define MEMORIA_CORE_PACKED_ALLOCATOR_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <memoria/core/packed2/packed_allocator_types.hpp>
#include <memoria/core/packed2/packed_fse_tree.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/dump.hpp>

#include <type_traits>

namespace memoria {

using namespace std;

enum class PackedBlockType {
	RAW_MEMORY = 0, ALLOCATABLE = 1
};

class PackedAllocator: public PackedAllocatable {

	typedef PackedAllocatable													Base;
	typedef PackedAllocator														MyType;
	typedef PackedAllocator														Allocator;

	typedef PackedFSETreeTypes <Int, Int, Int>									LayoutTypes;

public:
	typedef PackedFSETree<LayoutTypes>											Layout;

	typedef UBigInt																Bitmap;

private:
	Int block_size_;
	Int layout_size_;
	Int bitmap_size_;
	UByte buffer_[];

public:
	PackedAllocator() {}



	bool is_allocatable(Int idx) const
	{
		const Bitmap* bmp = bitmap();
		return GetBit(bmp, idx);
	}

	Layout* layout() {
		return T2T<Layout*>(buffer_);
	}

	const Layout* layout() const {
		return T2T<const Layout*>(buffer_);
	}

	Bitmap* bitmap() {
		return T2T<Bitmap*>(buffer_ + layout_size_);
	}

	const Bitmap* bitmap() const {
		return T2T<const Bitmap*>(buffer_ + layout_size_);
	}

	Int allocated() const {
		return layout()->sum();
	}

	Int client_area() const {
		return block_size_ - sizeof(MyType) - layout_size_ - bitmap_size_;
	}

	Int free_space() const {
		 return client_area() - allocated();
	}

	Int elements() const {
		return layout()->size();
	}

	UByte* base() {
		return buffer_ + layout_size_ + bitmap_size_;
	}

	const UByte* base() const {
		return buffer_ + layout_size_ + bitmap_size_;
	}

	Int layout_size() const {
		return layout_size_;
	}

	Int bitmap_size() const {
		return bitmap_size_;
	}

	Int block_size() const {
		return block_size_;
	}

	void init(Int block_size, Int blocks)
	{
		block_size_ = roundDownBytesToAlignmentBlocks(block_size);

		Layout* layout = this->layout();

		layout_size_ = roundUpBytesToAlignmentBlocks(Layout::block_size(blocks));

		memset(layout, 0, layout_size_);

		layout->init(layout_size_);
		layout->size() = blocks;
		layout->reindex();

		bitmap_size_ = roundUpBitsToAlignmentBlocks(blocks);

		Bitmap* bitmap = this->bitmap();
		memset(bitmap, 0, bitmap_size_);
	}

	static Int block_size(Int client_area, Int blocks)
	{
		Int layout_size = roundUpBytesToAlignmentBlocks(Layout::block_size(blocks));
		Int bitmap_size = roundUpBitsToAlignmentBlocks(blocks);

		return sizeof(MyType) + layout_size + bitmap_size + roundUpBytesToAlignmentBlocks(client_area);
	}



	Int computeElementOffset(const void* element) const
	{
		const UByte* base_ptr = base();
		const UByte* elt_ptr = T2T<const UByte*>(element);

		size_t diff = T2T<size_t>(elt_ptr - base_ptr);

		return diff;
	}

	Int resizeBlock(const void* element, Int new_size)
	{
		MEMORIA_ASSERT(new_size, >, 0);

		Layout* layout 	= this->layout();
		Int offset 		= computeElementOffset(element);
		Int idx 		= layout->findLT(offset).idx();

		Int allocation_size = roundUpBytesToAlignmentBlocks(new_size);

		Int size		= layout->value(idx);
		Int delta 		= allocation_size - size;

		if (delta > 0)
		{
			if (delta > free_space())
			{
				enlarge(delta);
			}
		}

		moveElements(idx + 1, delta);

		layout->value(idx) = allocation_size;

		layout->reindex();

		return allocation_size;
	}


//	Int enlargeBlock(const void* element, Int new_size)
//	{
//		Layout* layout 	= this->layout();
//		Int offset 		= computeElementOffset(element);
//		Int idx 		= layout->findLT(offset).idx();
//
//		Int allocation_size = roundUpBytesToAlignmentBlocks(new_size);
//
//		Int size		= layout->value(idx);
//		Int delta 		= allocation_size - size;
//
//		MEMORIA_ASSERT(delta, >=, 0);
//
//		if (delta > free_space())
//		{
//			enlarge(delta);
//		}
//
//		moveElements(idx + 1, delta);
//
//		layout->value(idx) = allocation_size;
//
//		layout->reindex();
//
//		return allocation_size;
//	}


	Int element_offset(Int idx) const {
		return layout()->sum(idx);
	}

	Int element_size(Int idx) const {
		return layout()->value(idx);
	}

	Int element_size(const void* element_ptr) const
	{
		const Layout* layout = this->layout();

		Int offset 	= computeElementOffset(element_ptr);
		Int idx 	= layout->findLT(offset).idx();

		return layout->value(idx);
	}

	template <typename T>
	const T* get(Int idx) const
	{
		MEMORIA_ASSERT(element_size(idx), >, 0);
		return T2T<const T*>(base() + element_offset(idx));
	}

	template <typename T>
	T* get(Int idx)
	{
		MEMORIA_ASSERT(element_size(idx), >, 0);
		return T2T<T*>(base() + element_offset(idx));
	}

	bool is_empty(int idx) const
	{
		return element_size(idx) == 0;
	}

	AllocationBlock describe(Int idx)
	{
		const Layout* lt 	= layout();

		Int offset 	= lt->sum(idx);
		Int size	= lt->value(idx);

		return AllocationBlock(size, offset, base() + offset);
	}

	AllocationBlockConst describe(Int idx) const
	{
		const Layout* lt 	= layout();

		Int offset 	= lt->sum(idx);
		Int size	= lt->value(idx);

		return AllocationBlockConst(size, offset, base() + offset);
	}

	template <typename T>
	T* allocate(Int idx, Int block_size)
	{
		static_assert(is_base_of<PackedAllocatable, T>::value, "Only derived classes of PackedAllocatable "
														"should be instantiated this way");

		AllocationBlock block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

		T* object = block.cast<T>();

		object->init(block.size());

		return object;
	}

	template <typename T>
	T* allocate(Int idx)
	{
		static_assert(!is_base_of<PackedAllocatable, T>::value, "Only classes that are not derived from PackedAllocatable "
																"should be instantiated this way");

		AllocationBlock block = allocate(idx, sizeof(T), PackedBlockType::RAW_MEMORY);
		return block.cast<T>();
	}

	template <typename T>
	T* allocateArrayByLength(Int idx, Int length)
	{
		static_assert(!is_base_of<PackedAllocatable, T>::value, "Only classes that are not derived from PackedAllocatable "
				"should be instantiated this way");

		AllocationBlock block = allocate(idx, length, PackedBlockType::RAW_MEMORY);
		return block.cast<T>();
	}

	template <typename T>
	T* allocateArrayBySize(Int idx, Int size)
	{
		static_assert(!is_base_of<PackedAllocatable, T>::value, "Only classes that are not derived from PackedAllocatable "
				"should be instantiated this way");

		AllocationBlock block = allocate(idx, sizeof(T)*size, PackedBlockType::RAW_MEMORY);
		return block.cast<T>();
	}


	AllocationBlock allocate(Int idx, Int size, PackedBlockType type)
	{
		Int allocation_size = roundUpBytesToAlignmentBlocks(size);

		if (allocation_size > free_space())
		{
			enlarge(allocation_size - free_space());
		}

		moveElements(idx + 1, allocation_size);

		Int offset = element_offset(idx);

		layout()->value(idx) = allocation_size;
		layout()->reindex();

		setBlockType(idx, type);

		memset(base() + offset, 0, allocation_size);

		if (type == PackedBlockType::ALLOCATABLE)
		{
			PackedAllocatable* alc = T2T<PackedAllocatable*>(base() + offset);
			alc->setAllocatorOffset(this);
		}

		return AllocationBlock(allocation_size, offset, base() + offset);
	}

	void free(Int idx)
	{
		Layout* layout 	= this->layout();

		Int size 		= element_size(idx);

		moveElements(idx + 1, -size);

		layout->value(idx) = 0;
		layout->reindex();
	}

	void clear(Int idx)
	{
		auto block = describe(idx);
		memset(block.ptr(), 0, block.size());
	}

	void setBlockType(Int idx, PackedBlockType type)
	{
		Bitmap* bitmap = this->bitmap();
		SetBit(bitmap, idx, type == PackedBlockType::ALLOCATABLE);
	}

	void dump(ostream& out = cout) const
	{
		out<<"PackedAllocator Layout:"<<endl;
		layout()->dump(out);

		out<<"PackedAllocator Block Types Bitmap:"<<endl;
		const Bitmap* bitmap = this->bitmap();

		dumpSymbols<Bitmap>(out, layout()->size(), 1, [bitmap](Int idx){
			return GetBit(bitmap, idx);
		});
	}

	Int enlarge(Int delta)
	{
		return resize(block_size_ + roundUpBytesToAlignmentBlocks(delta));
	}


	Int resize(Int new_size)
	{
		if (allocator_offset() > 0)
		{
			Allocator* alloc = allocator();
			block_size_ = alloc->resizeBlock(this, new_size);
		}
		else if (new_size < block_size_)
		{
			if (new_size >= allocated() + (Int)sizeof(MyType) + layout_size_ + bitmap_size_)
			{
				block_size_ = new_size;
			}
			else {
				throw PackedOOMException(MA_SRC, SBuf()<<"Requested allocator size is too small: "
						<<new_size<<" bytes. Allocated = "<<allocated()<<" bytes.");
			}
		}
		else {
			throw PackedOOMException(MA_SRC, SBuf()<<"no space left in packed allocator: "
												   <<new_size<<" bytes. Allocated = "<<allocated()
												   <<" bytes, avaliable = "<<free_space()<<" bytes.");
		}

		return block_size_;
	}

	void forceResize(Int amount)
	{
		block_size_ += roundDownBytesToAlignmentBlocks(amount);
	}


	Int pack()
	{
		return resize(block_size_ - free_space());
	}

protected:

	void moveElements(Int start_idx, Int delta)
	{
		Layout* layout = this->layout();

		if (delta > 0)
		{
			for (Int idx = layout->size() - 1; idx >= start_idx; idx--)
			{
				moveElement(idx, delta);
			}
		}
		else {
			for (Int idx = start_idx; idx < layout->size(); idx++)
			{
				moveElement(idx, delta);
			}
		}
	}

	void moveElement(Int idx, Int delta)
	{
		AllocationBlock block = describe(idx);

		if (block.size() > 0)
		{
			UByte* ptr = block.ptr();

			CopyByteBuffer(ptr, ptr + delta, block.size());

			if (is_allocatable(idx))
			{
				PackedAllocatable* element = T2T<PackedAllocatable*>(ptr + delta);
				element->setAllocatorOffset(this);
			}
		}
	}
};

}


#endif
