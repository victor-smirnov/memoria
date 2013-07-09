
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_TREE_TOOLS_HPP_
#define MEMORIA_CORE_PACKED_TREE_TOOLS_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/packed2/packed_allocator_types.hpp>
#include <memoria/core/packed2/packed_tools.hpp>

namespace memoria {


template <typename Type, typename Value>
struct FSECodec {
	typedef Type BufferType;

	size_t length(const Type* buffer, size_t idx) const {return 1;}
	size_t length(Value value) const {return 1;}

	size_t decode(const Type* buffer, Value& value, size_t idx) const {
		value = buffer[idx];
		return 1;
	}

	size_t encode(Type* buffer, Value value, size_t idx) const
	{
		buffer[idx] = value;
		return 1;
	}
};

template <typename Value>
using ValueFSECodec = FSECodec<Value, Value>;


template <
	typename ValueType,
	typename IndexType 		= ValueType,
	Int Blocks_				= 1,
	template <typename> class CodecType = ValueFSECodec,

	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= PackedTreeBranchingFactor
>
struct Packed2TreeTypes {
	typedef ValueType       Value;
	typedef IndexType       IndexValue;

    typedef PackedAllocator	Allocator;

    static const Int Blocks                 = Blocks_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;

    static const Int ALIGNMENT_BLOCK        = 8;

    template <typename V>
    using Codec = CodecType<V>;
};



template <typename IndexKey, Int BranchingFactor, Int ValuesPerBranch>
class PackedTreeTools {

private:

	static const Int LEVELS_MAX             = 32;

public:

	PackedTreeTools() {}

	template <typename Functor>
	static void update_up(Int idx, Functor&& fn)
	{
		MEMORIA_ASSERT(idx, >=, 0);
		MEMORIA_ASSERT(idx, <=, fn.size());

		Int level_size      = fn.maxSize();
		Int level_start     = fn.indexSize();

		Int level 			= 0;

		do {
			level_size      = getIndexCellsNumberFor(level, level_size);
			level_start     -= level_size;

			if (level > 0) {
				idx /= BranchingFactor;
			}
			else {
				idx /= ValuesPerBranch;
			}

			fn(idx + level_start);
		}
		while (level_start > 0);
	}

	template <typename Functor>
	static void reindex(Functor& fn)
	{
		if (fn.indexSize() > 0)
		{
			Int level_size    = getIndexCellsNumberFor(0, fn.maxSize());
			Int level_start   = fn.indexSize() - level_size;

			fn.clearIndex(0, fn.indexSize());

			fn.buildFirstIndexLine(level_start, level_size);

			if (fn.maxSize() == 0) {
				return;
			}

			while (level_start > 0)
			{
				Int parent_size 	= getIndexCellsNumberFor(1, level_size);
				Int parent_start 	= level_start - parent_size;

				for (Int idx = 0; idx < level_size; idx += BranchingFactor)
				{
					Int max = (idx + BranchingFactor) < level_size ? (idx + BranchingFactor) : level_size;
					fn.processIndex(parent_start + idx / BranchingFactor, idx + level_start, level_start + max);
				}

				level_start -= parent_size;
				level_size = parent_size;
			}

		}
		else {
			Int level_size    = getIndexCellsNumberFor(0, fn.maxSize());
			Int level_start   = fn.indexSize() - level_size;

			fn.buildFirstIndexLine(level_start, level_size);
		}
	}

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
	template <typename Functor>
	static void walk_range(Int start, Int end, Functor& walker)
	{
		MEMORIA_ASSERT(start, >=, 0);
		MEMORIA_ASSERT(end,   <=,  walker.size());
		MEMORIA_ASSERT(start, <=,  end);

		FinishHandler<Functor> finish_handler(walker);

		if (end - start <= ValuesPerBranch * 2)
		{
			walker.walkValues(start, end);
		}
		else {
			Int block_start_end     = getBlockStartEndV(start);
			Int block_end_start     = getBlockStartV(end);

			walker.walkValues(start, block_start_end);

			if (block_start_end < block_end_start)
			{
				Int level_size = getIndexCellsNumberFor(0, walker.max_size());
				walker.prepareIndex();
				walkIndexRange(
						start / ValuesPerBranch + 1,
						end / ValuesPerBranch,
						walker,
						walker.index_size() - level_size,
						level_size,
						ValuesPerBranch
				);
			}

			walker.walkValues(block_end_start, end);
		}
	}

	template <typename Functor>
	static void walk_range(Int target, Functor& walker)
	{
		if (target > walker.size()) {
			int a = 0; a++;
		}

		MEMORIA_ASSERT(target, <=, walker.size());

		FinishHandler<Functor> finish_handler(walker);

		Int levels = 0;
		Int level_sizes[LEVELS_MAX];

		Int level_size = walker.max_size();
		Int cell_size = 1;

		do
		{
			level_size = getIndexCellsNumberFor(levels, level_size);
			level_sizes[levels++] = level_size;
		}
		while (level_size > 1);

		cell_size = ValuesPerBranch;
		for (Int c = 0; c < levels - 2; c++)
		{
			cell_size *= BranchingFactor;
		}

		Int base = 1, start = 0, target_idx = target;

		for (Int level = levels - 2; level >= 0; level--)
		{
			Int end = target_idx / cell_size;

			walker.walkIndex(start + base, end + base, 0);

			start 		= level > 0 ? end * BranchingFactor : end * ValuesPerBranch;
			base 		+= level_sizes[level];
			cell_size 	/= BranchingFactor;
		}

		walker.walkValues(start, target);
	}


	template <typename Functor>
	static Int find_fw(Functor &walker)
	{
		FinishHandler<Functor> finish_handler(walker);

		if (walker.index_size() == 0)
		{
			return walker.walkLastValuesBlock(0);
		}
		else {

			Int levels = 0;
			Int level_sizes[LEVELS_MAX];

			Int level_size = walker.max_size();

			do
			{
				level_size = getIndexCellsNumberFor(levels, level_size);
				level_sizes[levels++] = level_size;
			}
			while (level_size > 1);

			Int base = 1, start = 0;

			for (Int level = levels - 2; level >= 0; level--)
			{
				Int level_size  = level_sizes[level];
				Int end         = (start + BranchingFactor < level_size) ? (start + BranchingFactor) : level_size;

				Int idx = walker.walkIndex(start + base, end + base, 0) - base;
				if (idx < end)
				{
					start = level > 0 ? idx * BranchingFactor : idx;
				}
				else {
					return walker.size();
				}

				base += level_size;
			}

			return walker.walkLastValuesBlock(start);
		}
	}



	template <typename Functor>
	static Int find_fw(Int start, Functor& walker)
	{
		Int size = walker.size();

		MEMORIA_ASSERT(start, <=, size);

		FinishHandler<Functor> finish_handler(walker);

		Int block_limit     = getBlockStartEndV(start);

		if (block_limit >= size)
		{
			return walker.walkValues(start, size);
		}
		else
		{
			Int limit = walker.walkFirstValuesBlock(start, block_limit);
			if (limit < block_limit)
			{
				return limit;
			}
			else {
				walker.prepareIndex();

				Int level_size      = getIndexCellsNumberFor(0, walker.max_size());
				Int level_limit     = getIndexCellsNumberFor(0, size);
				Int last_start      = walkIndexFw(
						block_limit/ValuesPerBranch,
						walker,
						walker.index_size() - level_size,
						level_size,
						level_limit,
						ValuesPerBranch
				);

				return walker.walkLastValuesBlock(last_start);
			}
		}
	}

	template <typename Functor>
	static Int find_bw(Int start, Functor& walker)
	{
		MEMORIA_ASSERT(start, >=, 0);

		FinishHandler<Functor> finish_handler(walker);

		Int block_end   = getBlockStartEndBwV(start);

		if (block_end == 0)
		{
			return walker.walkValues(start, 0);
		}
		else
		{
			Int limit = walker.walkValues(start, block_end);
			if (walker.is_found())
			{
				return limit;
			}
			else {
				walker.prepareIndex();

				Int level_size = getIndexCellsNumberFor(0, walker.max_size());
				Int last_start = walkIndexBw(
						block_end/ValuesPerBranch - 1,
						walker,
						walker.index_size() - level_size,
						level_size,
						ValuesPerBranch,
						ValuesPerBranch
				);

				if (last_start > 0)
				{
					return walker.walkValues(last_start, last_start - ValuesPerBranch);
				}
				else {
					return 0;
				}
			}
		}
	}



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


	template <typename Functor>
	static void walkIndexRange(Int start, Int end, Functor& walker, Int level_offet, Int level_size, Int cell_size)
	{
		if (end - start <= BranchingFactor * 2)
		{
			walker.walkIndex(start + level_offet, end + level_offet);
		}
		else {
			Int block_start_end     = getBlockStartEnd(start);
			Int block_end_start     = getBlockStart(end);

			walker.walkIndex(start + level_offet, block_start_end + level_offet);

			if (block_start_end < block_end_start)
			{
				Int level_size0 = getIndexCellsNumberFor(level_size);
				walkIndexRange(
						start / BranchingFactor + 1,
						end / BranchingFactor,
						walker,
						level_offet - level_size0,
						level_size0,
						BranchingFactor
				);
			}

			walker.walkIndex(block_end_start + level_offet, end + level_offet);
		}
	}

	template <typename Functor>
	static Int walkIndexFw(
			Int start,
			Functor& walker,
			Int level_offet,
			Int level_size,
			Int level_limit,
			Int cell_size
	)
	{
		Int block_start_end     = getBlockStartEnd(start);

		if (block_start_end >= level_limit)
		{
			return (walker.walkIndex(
					start + level_offet,
					level_limit + level_offet,
					cell_size
			)
			- level_offet);
		}
		else
		{
			Int limit = walker.walkIndex(start + level_offet, block_start_end + level_offet, cell_size) - level_offet;
			if (limit < block_start_end)
			{
				return limit;
			}
			else {
				Int level_size0     = getIndexCellsNumberFor(level_size);
				Int level_limit0    = getIndexCellsNumberFor(level_limit);

				Int last_start      = walkIndexFw(
						block_start_end / BranchingFactor,
						walker,
						level_offet - level_size0,
						level_size0,
						level_limit0,
						cell_size * BranchingFactor
				) * BranchingFactor;

				Int last_start_end  = getBlockStartEnd(last_start);

				Int last_end = last_start_end <= level_limit ? last_start_end : level_limit;

				return (walker.walkIndex(
						last_start + level_offet,
						last_end + level_offet,
						cell_size
				)
				- level_offet);
			}
		}
	}

	template <typename Functor>
	static Int walkIndexBw(
			Int start,
			Functor& walker,
			Int level_offet,
			Int level_size,
			Int cells_number_on_lower_level,
			Int cell_size
	)
	{
		Int block_start_end     = getBlockStartEndBw(start);

		if (block_start_end == -1)
		{
			return (walker.walkIndex(
					start + level_offet,
					level_offet - 1,
					cell_size
			)
			- level_offet + 1) * cells_number_on_lower_level;
		}
		else
		{
			Int idx = walker.walkIndex(start + level_offet, block_start_end + level_offet, cell_size) - level_offet;
			if (idx > block_start_end)
			{
				return (idx + 1) * cells_number_on_lower_level;
			}
			else {
				Int level_size0 = getIndexCellsNumberFor(level_size);
				Int last_start  = walkIndexBw(
						block_start_end / BranchingFactor,
						walker,
						level_offet - level_size0,
						level_size0,
						BranchingFactor,
						cell_size * BranchingFactor
				) - 1;

				Int last_start_end = getBlockStartEndBw(last_start);

				return (walker.walkIndex(
						last_start + level_offet,
						last_start_end + level_offet,
						cell_size
				)
				- level_offet + 1) * cells_number_on_lower_level;
			}
		}
	}

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
};

template <typename MyType>
class Reindex2FnBase {
	public:
		static const Int Indexes        		= MyType::Indexes;

		typedef typename MyType:: IndexValue 	IndexValue;

	protected:
		MyType& me_;

		IndexValue* indexes_[Indexes];

	public:
		Reindex2FnBase(MyType& me): me_(me)
		{
			for (Int idx = 0; idx < Indexes; idx++)
			{
				indexes_[idx] = me.indexes(idx);
			}
		}

		Int size() const {
			return me_.size();
		}

		Int maxSize() const {
			return me_.data_size();
		}

		Int indexSize() const {
			return me_.index_size();
		}

		void clearIndex(Int start, Int end)
		{
			for (Int idx = 0; idx < Indexes; idx++)
			{
				for (Int c = start; c < end; c++)
				{
					indexes_[idx][c] = 0;
				}
			}
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
class Check2FnBase {
public:
	static const Int Indexes        			= MyType::Indexes;

	typedef typename MyType:: IndexValue 		IndexValue;

protected:
	const MyType& me_;

	const IndexValue* indexes_[Indexes];

public:
	Check2FnBase(const MyType& me): me_(me)
	{
		for (Int idx = 0; idx < Indexes; idx++)
		{
			indexes_[idx] = me.indexes(idx);
		}
	}

	Int size() const {
		return me_.size();
	}

	Int maxSize() const {
		return me_.data_size();
	}

	Int indexSize() const {
		return me_.index_size();
	}

	void clearIndex(Int start, Int end)
	{}

	void processIndex(Int parent, Int start, Int end)
	{
		for (Int idx = 0; idx < Indexes; idx++)
		{
			IndexValue sum = 0;

			for (Int c = start; c < end; c++)
			{
				sum += indexes_[idx][c];
			}

			MEMORIA_ASSERT(indexes_[idx][parent], ==, sum);
		}
	}
};


}


#endif
