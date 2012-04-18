
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PMAP_PACKED_SUM_TREE_HPP_
#define MEMORIA_CORE_PMAP_PACKED_SUM_TREE_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/traits.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/pmap/packed_tree.hpp>
#include <memoria/core/pmap/tree_walkers.hpp>

namespace memoria {

template <typename Types>
class PackedSumTree: public PackedTree<Types> {

	typedef PackedTree<Types>				Base;
	typedef PackedSumTree<Types>			MyType;

public:

	typedef typename Base::Key				Key;
	typedef typename Base::IndexKey			IndexKey;
	typedef typename Base::Value			Value;
	typedef typename Base::Accumulator		Accumulator;

	static const Int Blocks					= Base::Blocks;
	static const Int BranchingFactor		= Base::BranchingFactor;

private:


public:
	PackedSumTree(): Base() {}




	void Reindex(Int block_num)
	{
		Reindex(block_num, 0, Base::size());
	}

	void ReindexAll(Int start, Int end)
	{
		for (Int c = 0; c < Blocks; c++)
		{
			Reindex(c, start, end);
		}
	}

	void Reindex(Int block_num, Int start, Int end)
	{
		Int block_start = Base::GetBlockStart(start);
		Int block_end 	= Base::GetBlockEnd(end);

		Int index_block_offset 	= Base::GetIndexKeyBlockOffset(block_num);
		Int key_block_offset 	= Base::GetKeyBlockOffset(block_num);

		Int index_level_size	= Base::GetIndexCellsNumberFor(Base::max_size());
		Int index_level_start 	= Base::index_size() - index_level_size;

		Int level_max 			= Base::size();

		for (Int c = block_start; c < block_end; c += BranchingFactor)
		{
			IndexKey sum = 0;
			Int max 	 = c + BranchingFactor <= level_max ? c + BranchingFactor : level_max;

			for (Int d = c; d < max; d++)
			{
				sum += Base::keyb(key_block_offset, d);
			}

			Int idx = c / BranchingFactor + index_level_start;
			Base::indexb(index_block_offset, idx) = sum;
		}

		while (index_level_start > 0)
		{
			level_max 		= Base::GetIndexCellsNumberFor(level_max);
			block_start 	= Base::GetBlockStart(block_start / BranchingFactor);
			block_end 		= Base::GetBlockEnd(block_end / BranchingFactor);

			Int index_parent_size 	= Base::GetIndexCellsNumberFor(index_level_size);
			Int index_parent_start	= index_level_start - index_parent_size;

			for (Int c = block_start; c < block_end; c += BranchingFactor)
			{
				IndexKey sum = 0;
				Int max 	 = (c + BranchingFactor <= level_max ? c + BranchingFactor : level_max) + index_level_start;

				for (Int d = c + index_level_start; d < max; d++)
				{
					sum += Base::indexb(index_block_offset, d);
				}

				Int idx = c / BranchingFactor + index_parent_start;
				Base::indexb(index_block_offset, idx) = sum;
			}

			index_level_size 	= index_parent_size;
			index_level_start 	-= index_parent_size;
		}
	}


	Int FindLE(Int block_num, const Key& k) const
	{
		LESumComparator<Key, IndexKey> cmp;
		return Base::Find(block_num, k, cmp);
	}

	Int FindLE(Int block_num, const Key& k, Accumulator& acc) const
	{
		LESumComparator<Key, IndexKey> cmp;
		Int result = Base::Find(block_num, k, cmp);
		acc[block_num] += cmp.sum();
		return result;
	}

	Int FindLT(Int block_num, const Key& k) const
	{
		LTSumComparator<Key, IndexKey> cmp;
		return Base::Find(block_num, k, cmp);
	}

	Int FindLT(Int block_num, const Key& k, Accumulator& acc) const
	{
		LTSumComparator<Key, IndexKey> cmp;
		Int result = Base::Find(block_num, k, cmp);
		acc[block_num] += cmp.sum();
		return result;
	}


	Int FindEQ(Int block_num, const Key& k) const
	{
		EQSumComparator<Key, IndexKey> cmp;
		return Base::Find(block_num, k, cmp);
	}

	Int FindEQ(Int block_num, const Key& k, Accumulator& acc) const
	{
		EQSumComparator<Key, IndexKey> cmp;
		Int result = Base::Find(block_num, k, cmp);
		acc[block_num] += cmp.sum();
		return result;
	}


	void Sum(Int block_num, Int start, Int end, Accumulator& accum) const
	{
		SumWalker<MyType, Key, IndexKey, Blocks> walker(*this, block_num);
		Base::WalkRange(start, end, walker);
		accum[block_num] += walker.sum();
	}

	Int FindSumPositionFw(Int block_num, Int start, Key key, Accumulator& acc) const
	{
		FindSumPositionFwFn<MyType, Key, IndexKey, Blocks> walker(*this, block_num, key);
		return Base::WalkFw(start, walker);
	}

	Int FindSumPositionBw(Int block_num, Int start, Key key, Accumulator& acc) const
	{
		FindSumPositionBwFn<MyType, Key, IndexKey, Blocks> walker(*this, block_num, key);
		return Base::WalkBw(start, walker);
	}

};

}


#endif
