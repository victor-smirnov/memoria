
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

#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria {

template <typename Allocator_ = PackedAllocator>
struct PackedWaveletTreeTypes {
	typedef Allocator_ 				Allocator;
};

template <typename Types>
class PackedWaveletTree: public PackedAllocator {

	typedef PackedAllocator														Base;
	typedef PackedWaveletTree<Types>											MyType;

	typedef typename Types::Allocator 											Allocator;


public:
	typedef PackedBitVector<
		PackedBitVectorTypes<>
	>																			BitVector;

	typedef PackedFSEArray<
		PackedFSEArrayTypes<
			UByte
		>
	>																			NodeLabelsArray;

	typedef PackedFSETree<
		PackedFSETreeTypes<
			Int,
			Int,
			Int
		>
	>																			SequenceLebelsArray;


	typedef PackedFSECxSequence<
		PackedFSECxSequenceTypes<
			8,
			UByte,
			PackedTreeBranchingFactor,
			512
		>
	>																			Sequence;


	class Metadata {
		Int size_;
		Int max_size_;
	public:
		Int& size() 		{return size_;}
		Int& max_size() 	{return max_size_;}

		const Int& size() const 	{return size_;}
		const Int& max_size() const {return max_size_;}
	};

public:

	PackedWaveletTree() {}

	Metadata* metadata()
	{
		return Base::template get<Metadata>(0);
	}

	const Metadata* metadata() const
	{
		return Base::template get<Metadata>(0);
	}

	BitVector* bit_vector()
	{
		return Base::template get<BitVector>(1);
	}

	const BitVector* bit_vector() const
	{
		return Base::get<BitVector>(1);
	}

	NodeLabelsArray* node_label_array()
	{
		return Base::template get<NodeLabelsArray>(2);
	}

	const NodeLabelsArray* node_label_array() const
	{
		return Base::template get<NodeLabelsArray>(2);
	}

	SequenceLebelsArray* sequence_label_array()
	{
		return Base::template get<SequenceLebelsArray>(3);
	}

	const SequenceLebelsArray* sequence_label_array() const
	{
		return Base::template get<SequenceLebelsArray>(3);
	}

	Sequence* sequence()
	{
		return Base::template get<Sequence>(4);
	}

	const Sequence* sequence() const
	{
		return Base::template get<Sequence>(4);
	}

	void init(Int block_size)
	{
		Base::init(block_size, 5);

		Int bitvector_size 			= roundBytesToAlignmentBlocks(block_size / 20);
		Int node_labels_size 		= roundBytesToAlignmentBlocks(block_size / 5);
		Int sequence_labels_size 	= roundBytesToAlignmentBlocks(block_size / 20);

		Int sequence_size 			= Base::client_area()
									- bitvector_size
									- node_labels_size
									- sequence_labels_size
									- roundBytesToAlignmentBlocks(sizeof(Metadata));

		allocateStruct<Metadata>				(0, sizeof(Metadata));
		allocateElement<BitVector>				(1, bitvector_size);
		allocateElement<NodeLabelsArray>		(2, node_labels_size);
		allocateElement<SequenceLebelsArray>	(3, sequence_labels_size);

		allocateElement<Sequence>				(4, sequence_size);
	}

	static Int block_size(Int client_area)
	{
		return Base::block_size(client_area - roundBytesToAlignmentBlocks(sizeof(Metadata)), 5);
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
		AllocationBlock block = Base::allocate(idx, block_size);

		if (block)
		{
			Base::setBlockType(idx, PackedBlockType::ALLOCATABLE);

			T* element = T2T<T*>(block.ptr());
			element->init(block.size());
			element->setAllocatorOffset(this);
		}
		else {
			throw Exception(MA_SRC, SBuf()<<"Can't allocate object Wavelet Tree content block "<<idx);
		}
	}

	template <typename T>
	void allocateStruct(Int idx, Int block_size)
	{
		AllocationBlock block = Base::allocate(idx, block_size);

		if (block)
		{
			Base::setBlockType(idx, PackedBlockType::RAW_MEMORY);
		}
		else {
			throw Exception(MA_SRC, SBuf()<<"Can't allocate object Wavelet Tree content block "<<idx);
		}
	}
};


}


#endif
