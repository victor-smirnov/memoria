
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_TREE_BASE_HPP_
#define MEMORIA_CORE_PACKED_TREE_BASE_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/packed/packed_allocator_types.hpp>
#include <memoria/core/packed/packed_tools.hpp>

namespace memoria {
/*

template <typename MyType, typename IndexKey, Int BranchingFactor, Int ValuesPerBranch>
class PackedTreeBase: public PackedAllocatable {

private:

	static const Int LEVELS_MAX             = 32;

protected:

	PackedTreeBase() {}

protected:

//	template <typename Functor>
//	void reindex(Functor&& fn) const
//	{
//		if (fn.indexSize() > 0)
//		{
//			Int level_size    = getIndexCellsNumberFor(0, fn.maxSize());
//			Int level_start   = fn.indexSize() - level_size;
//
//			fn.clearIndex(0, fn.indexSize());
//
//			fn.buildFirstIndexLine(level_start, level_size);
//
//			while (level_start > 0)
//			{
//				Int parent_size 	= getIndexCellsNumberFor(1, level_size);
//				Int parent_start 	= level_start - parent_size;
//
//				for (Int idx = 0; idx < level_size; idx += BranchingFactor)
//				{
//					Int max = (idx + BranchingFactor) < level_size ? (idx + BranchingFactor) : level_size;
//					fn.processIndex(parent_start + idx / BranchingFactor, idx + level_start, level_start + max);
//				}
//
//				level_start -= parent_size;
//				level_size = parent_size;
//			}
//
//		}
//	}

private:
	template <typename Walker>
	class FinishHandler {
		Walker& walker_;
	public:
		FinishHandler(Walker& walker): walker_(walker) {}

		~FinishHandler()
		{
			walker_.finish();
		}
	};

public:

protected:

	static Int getBlockStart(Int i)
	{
		return (i / BranchingFactor) * BranchingFactor;
	}

	static Int getBlockStartEnd(Int i)
	{
		return (i / BranchingFactor + 1) * BranchingFactor;
	}

	static Int getBlockStartV(Int i)
	{
		return (i / ValuesPerBranch) * ValuesPerBranch;
	}

	static Int getBlockStartEndV(Int i)
	{
		return (i / ValuesPerBranch + 1) * ValuesPerBranch;
	}


	static Int getBlockStartEndBw(Int i)
	{
		return (i / BranchingFactor) * BranchingFactor - 1;
	}


	static Int getBlockStartEndBwV(Int i)
	{
		return (i / ValuesPerBranch) * ValuesPerBranch;
	}


	static Int getBlockEnd(Int i)
	{
		return (i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0)) * BranchingFactor;
	}


	static Int getBlockEndV(Int i)
	{
		return (i / ValuesPerBranch + ((i % ValuesPerBranch) ? 1 : 0)) * ValuesPerBranch;
	}

	static Int getIndexCellsNumberFor(Int i)
	{
		return getIndexCellsNumberFor(1, i);
	}

	static Int getIndexCellsNumberFor(Int level, Int i)
	{
		if (level > 0)
		{
			return i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0);
		}
		else {
			return i / ValuesPerBranch + ((i % ValuesPerBranch) ? 1 : 0);
		}
	}



	MyType& me() {
		return *static_cast<MyType*>(this);
	}

	const MyType& me() const {
		return *static_cast<const MyType*>(this);
	}

protected:

	static Int compute_index_size(Int csize)
	{
		if (csize == 1)
		{
			return 1;
		}
		else {
			Int sum = 0;
			for (Int nlevels=0; csize > 1; nlevels++)
			{
				if (nlevels > 0) {
					csize = ((csize % BranchingFactor) == 0) ?
								(csize / BranchingFactor) :
									(csize / BranchingFactor) + 1;
				}
				else {
					csize = ((csize % ValuesPerBranch) == 0) ?
								(csize / ValuesPerBranch) :
									(csize / ValuesPerBranch) + 1;
				}
				sum += csize;
			}
			return sum;
		}
	}

	static Int roundBytesToAlignmentBlocks(Int value)
	{
		return (value / AlignmentBlock + (value % AlignmentBlock ? 1 : 0)) * AlignmentBlock;
	}

	static Int roundBitsToBytes(Int bitsize)
	{
		return (bitsize / 8 + (bitsize % 8 ? 1 : 0));
	}

	static Int roundBitsToAlignmentBlocks(Int bitsize)
	{
		Int byte_size = roundBitsToBytes(bitsize);
		return roundBytesToAlignmentBlocks(byte_size);
	}

	template <typename Fn>
	static Int getMaxSize(Int block_size, Fn&& fn)
	{
		return FindTotalElementsNumber(block_size, fn);
	}
};

*/

template <typename MyType>
class ReindexFnBase {
public:
	static const Int Indexes        		= MyType::Indexes;

	typedef typename MyType::IndexValue 	IndexValue;

protected:
	MyType& me_;

	IndexValue* indexes_[Indexes];

public:
	ReindexFnBase(MyType& me): me_(me)
	{
		for (Int idx = 0; idx < Indexes; idx++)
		{
			indexes_[idx] = me.indexes(idx);
		}

		Int end = me_.index_size();

		for (Int idx = 0; idx < Indexes; idx++)
		{
			for (Int c = 0; c < end; c++)
			{
				indexes_[idx][c] = 0;
			}
		}
	}

	MyType& tree(){
		return me_;
	}

	void processIndex(Int parent, Int start, Int end)
	{
		for (Int idx = 0; idx < Indexes; idx++)
		{
			IndexValue sum = 0;

			for (Int c = start; c < end; c++)
			{
				sum += indexes_[idx][c];
			}

			indexes_[idx][parent] = sum;
		}
	}
};



template <typename MyType>
class CheckFnBase {
public:
	static const Int Indexes        		= MyType::Indexes;

	typedef typename MyType:: IndexValue	IndexValue;

protected:
	const MyType& me_;

	const IndexValue* indexes_[Indexes];

public:
	CheckFnBase(const MyType& me): me_(me)
	{
		for (Int idx = 0; idx < Indexes; idx++)
		{
			indexes_[idx] = me.indexes(idx);
		}
	}

	const MyType& tree() const {
		return me_;
	}

	void processIndex(Int parent, Int start, Int end)
	{
		for (Int idx = 0; idx < Indexes; idx++)
		{
			IndexValue sum = 0;

			for (Int c = start; c < end; c++)
			{
				sum += indexes_[idx][c];
			}

			if (indexes_[idx][parent] != sum)
			{
				throw Exception(MA_SRC,
						SBuf()<<"Invalid index: index["<<idx<<"]["<<parent<<"]="<<indexes_[idx][parent]<<", actual="<<sum);
			}
		}
	}
};




}


#endif
