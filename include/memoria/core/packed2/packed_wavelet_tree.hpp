
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_WAVELET_TREE_HPP_
#define MEMORIA_CORE_PACKED_WAVELET_TREE_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/core/packed2/packed_dynamic_allocator.hpp>

#include <memoria/core/packed2/packed_fse_cxsequence.hpp>
#include <memoria/core/packed2/packed_bitvector.hpp>
#include <memoria/core/packed2/packed_fse_tree.hpp>

namespace memoria {

template <typename Allocator_>
struct PackedWaveletTreeTypes {
	typedef Allocator_ 				Allocator;
};

template <typename Types>
class PackedWaveletTree: public PackedAllocatable {

	typedef PackedAllocatable													Base;
	typedef PackedWaveletTree<Types>											MyType;

	typedef typename Types::Allocator 											Allocator;
	typedef PackedDynamicAllocator<MyType>										ContentAllocator;

public:
	typedef PackedBitVector<
		PackedBitVectorTypes<
			ContentAllocator
		>
	>																			BitVector;

	typedef PackedFSEArray<
		PackedFSEArrayTypes<
			UByte,
			ContentAllocator
		>
	>																			NodeLabelsArray;

	typedef PackedFSETree<
		PackedFSETreeTypes<
			Int,
			Int,
			Int,
			ContentAllocator
		>
	>																			SequenceLebelsArray;


	typedef PackedFSECxSequence<
		PackedFSECxSequenceTypes<
			8,
			UByte,
			ContentAllocator,
			PackedTreeBranchingFactor,
			512
		>
	>																			Sequence;


private:
	Int 		size_;
	UByte buffer_[];

public:

	PackedWaveletTree() {}

	ContentAllocator* content_allocator() {
		return T2T<ContentAllocator*>(buffer_);
	}

	const ContentAllocator* content_allocator() const {
		return T2T<const ContentAllocator*>(buffer_);
	}

	Allocator* allocator()
	{
		if (Base::allocator_offset() > 0)
		{
			UByte* my_ptr = T2T<UByte*>(this);
			return T2T<Allocator*>(my_ptr - Base::allocator_offset());
		}
		else {
			return nullptr;
		}
	}

	const Allocator* allocator() const
	{
		if (Base::allocator_offset() > 0)
		{
			const UByte* my_ptr = T2T<const UByte*>(this);
			return T2T<const Allocator*>(my_ptr - Base::allocator_offset());
		}
		else {
			return nullptr;
		}
	}

	BitVector* bit_vector()
	{
		return content_allocator()->template get<BitVector>(0);
	}

	const BitVector* bit_vector() const
	{
		return allocator()->template get<BitVector>(0);
	}

	NodeLabelsArray* node_label_array()
	{
		return content_allocator()->template get<NodeLabelsArray>(1);
	}

	const NodeLabelsArray* node_label_array() const
	{
		return allocator()->template get<NodeLabelsArray>(1);
	}

	SequenceLebelsArray* sequence_label_array()
	{
		return content_allocator()->template get<SequenceLebelsArray>(2);
	}

	const SequenceLebelsArray* sequence_label_array() const
	{
		return allocator()->template get<SequenceLebelsArray>(2);
	}

	Sequence* sequence()
	{
		return content_allocator()->template get<Sequence>(3);
	}

	const Sequence* sequence() const
	{
		return allocator()->template get<Sequence>(3);
	}

	void init(Int block_size)
	{
		content_allocator()->init(block_size - sizeof(MyType), 4);
		content_allocator()->setAllocatorOffset(this);

		Int bitvector_size 			= block_size / 20;
		Int node_labels_size 		= block_size / 5;
		Int sequence_labels_size 	= block_size / 20;

		Int sequence_size 			= block_size - bitvector_size - node_labels_size - sequence_labels_size;

		allocateElement<BitVector>				(0, bitvector_size);
		allocateElement<NodeLabelsArray>		(1, node_labels_size);
		allocateElement<SequenceLebelsArray>	(2, sequence_labels_size);
		allocateElement<Sequence>				(3, sequence_size);
	}

	Int block_size() const
	{
		return 0;
	}

	Int enlargeBlock(void* element, Int new_size)
	{
		Allocator* alloc = allocator();
		if (alloc)
		{
			return alloc->enlargeBlock(this, new_size);
		}
		else {
			return 0;
		}
	}

private:

	template <typename T>
	void allocateElement(Int idx, Int block_size)
	{
		AllocationBlock block = content_allocator()->allocate(idx, block_size);

		T* element = T2T<T*>(block.ptr());
		element->init(block.size());
		element->setAllocatorOffset(content_allocator());
	}
};


}


#endif
