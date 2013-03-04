
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_TREE_HPP_
#define MEMORIA_CORE_PACKED_VLE_TREE_HPP_

#include <memoria/core/packed/packed_tree_base.hpp>

#include <memoria/core/tools/accessors.hpp>

namespace memoria {

struct EmptyResizeHandler {

};



template <
	typename K,
	typename IK,
	typename V,
	typename ResizeHandler_ = EmptyResizeHandler,
	Int Blocks_ = 1,
	Int BF = PackedTreeBranchingFactor
>
struct PackedVLETreeTypes {
    typedef K               Key;
    typedef IK              IndexKey;
    typedef V               Value;
    typedef ResizeHandler_  ResizeHandler;

    static const Int Blocks                 = Blocks_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = BF;
};



template <typename Types>
class PackedVLETree: public PackedTreeBase<PackedVLETree<Types>, Types> {
public:
	static const UInt VERSION               									= 1;

	typedef PackedVLETree<Types>               									MyType;

	typedef typename Types::ResizeHandler										ResizeHandler;

	typedef typename Types::Key													Key;
	typedef typename Types::IndexKey											IndexKey;

	typedef UBigInt																OffsetsType;

	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Blocks        			= Types::Blocks;

	static const Int BITS_PER_OFFSET		= 4;
	static const Int ALIGNMENT_BLOCK		= 8; //Bytes

private:
	Int size_;
	Int index_size_;
	Int max_size_;
	Int block_size_;

	UByte buffer_[];

public:
	PackedVLETree() {}

	Int& size() {return size_;}
	const Int& size() const {return size_;}

	const Int& index_size() const {return index_size_;}

	const Int& max_size() const {return max_size_;}
	const Int& block_size() const {return block_size_;}

	OffsetsType* offsetsBlock() {
		return T2T<OffsetsType*> (buffer_);
	}

	const OffsetsType* offsetsBlock() const {
		return T2T<OffsetsType*> (buffer_);
	}

	BitmapAccessor<OffsetsType*, Int, BITS_PER_OFFSET>
	offset(Int idx)
	{
		return BitmapAccessor<OffsetsType*, Int, BITS_PER_OFFSET>(offsetsBlock(), idx);
	}

	BitmapAccessor<const OffsetsType*, Int, BITS_PER_OFFSET>
	offset(Int idx) const
	{
		return BitmapAccessor<const OffsetsType*, Int, BITS_PER_OFFSET>(offsetsBlock(), idx);
	}

	static Int roundBitsToBytes(Int bitsize)
	{
		return (bitsize / 8 + (bitsize % 8 ? 1 : 0));
	}

	static Int roundBytesToAlignmentBlocks(Int value)
	{
		return (value / ALIGNMENT_BLOCK + (value % ALIGNMENT_BLOCK ? 1 : 0)) * ALIGNMENT_BLOCK;
	}

	static Int roundBitsToAlignmentBlocks(Int bitsize)
	{
		Int byte_size = roundBitsToBytes(bitsize);
		return roundBytesToAlignmentBlocks(byte_size);
	}

	static Int getBlockSize(Int items_num)
	{
		Int value_blocks = items_num / ValuesPerBranch + (items_num % ValuesPerBranch ? 1 : 0);

		Int offsets_bits = value_blocks * BITS_PER_OFFSET;

		Int offsets_bytes = roundBitsToAlignmentBlocks(offsets_bits);

		Int index_size = getIndexSize(items_num);

		return offsets_bytes + index_size * (Blocks + 1) + items_num;
	}

	static Int getIndexSize(Int items_number)
	{
		return MyType::template compute_index_size<MyType>(items_number);
	}

	class InitByBlockFn {
		const MyType& tree_;
	public:

		InitByBlockFn(const MyType& tree): tree_(tree) {}

		Int last(Int block_size) const {
			return block_size;
		}

		Int getBlockSize(Int items_number) const
		{
			return MyType::getBlockSize(items_number);
		}

		Int extend(Int items_number) const {
			return items_number;
		}

		Int getIndexSize(Int items_number) const {
			return MyType::template compute_index_size<MyType>(items_number);
		}
	};

	void initByBlock(Int block_size)
	{
		size_ = 0;

		max_size_   = MyType::getMaxSize(block_size, InitByBlockFn(*this));
		index_size_ = getIndexSize(max_size_);

		block_size_ = block_size;
	}


	void initSizes(Int max)
	{
		size_       = 0;
		max_size_   = max;
		index_size_ = getIndexSize(max_size_);
	}
};


}


#endif
