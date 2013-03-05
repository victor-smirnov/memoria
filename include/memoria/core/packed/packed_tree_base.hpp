
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_TREE_BASE_HPP_
#define MEMORIA_CORE_PACKED_TREE_BASE_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria {

template <typename MyType, typename Types>
class PackedTreeBase {

public:

	typedef typename Types::IndexKey        IndexKey;
	typedef typename Types::Value           Value;
	typedef typename Types::Value           Symbol;

	template <typename T> friend class PackedTree;

private:

	static const Int LEVELS_MAX             = 32;

protected:

	PackedTreeBase() {}

	template <typename Functor>
	void update_up(Int idx, Functor&& fn)
	{
		MEMORIA_ASSERT(idx, >=, 0);
		MEMORIA_ASSERT(idx, <=, fn.size());

		Int BranchingFactor = Functor::BranchingFactor;
		Int ValuesPerBranch = Functor::ValuesPerBranch;

		Int level_size      = fn.maxSize();
		Int level_start     = fn.indexSize();

		Int level 			= 0;

		do {
			level_size      = getIndexCellsNumberFor<Functor>(level, level_size);
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

protected:

	template <typename Functor>
	void reindex(Int start, Int end, Functor& fn)
	{
		MEMORIA_ASSERT(start, >=, 0);
		MEMORIA_ASSERT(end, <=, fn.size());
		MEMORIA_ASSERT(start, <=, end);

		Int BranchingFactor = Functor::BranchingFactor;

		Int level_size    = getIndexCellsNumberFor<Functor>(0, fn.maxSize());
		Int level_start   = fn.indexSize() - level_size;

		fn.clearIndex(0, fn.indexSize());

		fn.buildFirstIndexLine(level_start, level_size);

		while (level_start > 0)
		{
			Int parent_size 	= getIndexCellsNumberFor<Functor>(1, level_size);
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
	void walk_range(Int start, Int end, Functor& walker) const
	{
		MEMORIA_ASSERT(start, >=, 0);
		MEMORIA_ASSERT(end,   <=,  walker.size());
		MEMORIA_ASSERT(start, <=,  end);

		Int ValuesPerBranch = Functor::ValuesPerBranch;

		FinishHandler<Functor> finish_handler(walker);

		if (end - start <= ValuesPerBranch * 2)
		{
			walker.walkValues(start, end);
		}
		else {
			Int block_start_end     = getBlockStartEndV<Functor>(start);
			Int block_end_start     = getBlockStartV<Functor>(end);

			walker.walkValues(start, block_start_end);

			if (block_start_end < block_end_start)
			{
				Int level_size = getIndexCellsNumberFor<Functor>(0, walker.max_size());
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
	void walk_range(Int target, Functor& walker) const
	{
		MEMORIA_ASSERT(target,   <=,  walker.size());

		Int BranchingFactor = Functor::BranchingFactor;
		Int ValuesPerBranch = Functor::ValuesPerBranch;

		FinishHandler<Functor> finish_handler(walker);

		Int levels = 0;
		Int level_sizes[LEVELS_MAX];

		Int level_size = walker.max_size();
		Int cell_size = 1;

		do
		{
			level_size = getIndexCellsNumberFor<Functor>(levels, level_size);
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

			walker.walkIndex(start + base, end + base);

			start 		= level > 0 ? end * BranchingFactor : end * ValuesPerBranch;
			base 		+= level_sizes[level];
			cell_size 	/= BranchingFactor;
		}

		return walker.walkValues(start, target);
	}


	template <typename Functor>
	Int find_fw(Functor &walker) const
	{
		FinishHandler<Functor> finish_handler(walker);

		Int levels = 0;
		Int level_sizes[LEVELS_MAX];

		Int BranchingFactor = Functor::BranchingFactor;

		Int level_size = walker.max_size();

		do
		{
			level_size = getIndexCellsNumberFor<Functor>(levels, level_size);
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



	template <typename Functor>
	Int find_fw(Int start, Functor& walker) const
	{
		Int size = walker.size();

		MEMORIA_ASSERT(start, <=, size);

		Int ValuesPerBranch = Functor::ValuesPerBranch;

		FinishHandler<Functor> finish_handler(walker);

		Int block_limit     = getBlockStartEndV<Functor>(start);

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

				Int level_size      = getIndexCellsNumberFor<Functor>(0, walker.max_size());
				Int level_limit     = getIndexCellsNumberFor<Functor>(0, size);
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
	Int find_bw(Int start, Functor& walker) const
	{
		MEMORIA_ASSERT(start, >=, 0);

		Int ValuesPerBranch = Functor::ValuesPerBranch;

		FinishHandler<Functor> finish_handler(walker);

		Int block_end   = getBlockStartEndBwV<Functor>(start);

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

				Int level_size = getIndexCellsNumberFor<Functor>(0, walker.max_size());
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

private:

	template <typename Fn>
	static Int getBlockStart(Int i)
	{
		return (i / Fn::BranchingFactor) * Fn::BranchingFactor;
	}

	template <typename Fn>
	static Int getBlockStartEnd(Int i)
	{
		return (i / Fn::BranchingFactor + 1) * Fn::BranchingFactor;
	}

	template <typename Fn>
	static Int getBlockStartV(Int i)
	{
		return (i / Fn::ValuesPerBranch) * Fn::ValuesPerBranch;
	}

	template <typename Fn>
	static Int getBlockStartEndV(Int i)
	{
		return (i / Fn::ValuesPerBranch + 1) * Fn::ValuesPerBranch;
	}

	template <typename Fn>
	static Int getBlockStartEndBw(Int i)
	{
		return (i / Fn::BranchingFactor) * Fn::BranchingFactor - 1;
	}

	template <typename Fn>
	static Int getBlockStartEndBwV(Int i)
	{
		return (i / Fn::ValuesPerBranch) * Fn::ValuesPerBranch;
	}

	template <typename Fn>
	static Int getBlockEnd(Int i)
	{
		return (i / Fn::BranchingFactor + ((i % Fn::BranchingFactor) ? 1 : 0)) * Fn::BranchingFactor;
	}

	template <typename Fn>
	static Int getBlockEndV(Int i)
	{
		return (i / Fn::ValuesPerBranch + ((i % Fn::ValuesPerBranch) ? 1 : 0)) * Fn::ValuesPerBranch;
	}

	template <typename Fn>
	static Int getIndexCellsNumberFor(Int i)
	{
		return getIndexCellsNumberFor<Fn>(1, i);
	}

	template <typename Fn>
	static Int getIndexCellsNumberFor(Int level, Int i)
	{
		if (level > 0)
		{
			return i / Fn::BranchingFactor + ((i % Fn::BranchingFactor) ? 1 : 0);
		}
		else {
			return i / Fn::ValuesPerBranch + ((i % Fn::ValuesPerBranch) ? 1 : 0);
		}
	}



	MyType& me() {
		return *static_cast<MyType*>(this);
	}

	const MyType& me() const {
		return *static_cast<const MyType*>(this);
	}


	template <typename Functor>
	void walkIndexRange(Int start, Int end, Functor& walker, Int level_offet, Int level_size, Int cell_size) const
	{
		Int BranchingFactor = Functor::BranchingFactor;

		if (end - start <= BranchingFactor * 2)
		{
			walker.walkIndex(start + level_offet, end + level_offet);
		}
		else {
			Int block_start_end     = getBlockStartEnd<Functor>(start);
			Int block_end_start     = getBlockStart<Functor>(end);

			walker.walkIndex(start + level_offet, block_start_end + level_offet);

			if (block_start_end < block_end_start)
			{
				Int level_size0 = getIndexCellsNumberFor<Functor>(level_size);
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
	Int walkIndexFw(
			Int start,
			Functor& walker,
			Int level_offet,
			Int level_size,
			Int level_limit,
			Int cell_size
	) const
	{
		Int BranchingFactor = Functor::BranchingFactor;

		Int block_start_end     = getBlockStartEnd<Functor>(start);

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
				Int level_size0     = getIndexCellsNumberFor<Functor>(level_size);
				Int level_limit0    = getIndexCellsNumberFor<Functor>(level_limit);

				Int last_start      = walkIndexFw(
						block_start_end / BranchingFactor,
						walker,
						level_offet - level_size0,
						level_size0,
						level_limit0,
						cell_size * BranchingFactor
				) * BranchingFactor;

				Int last_start_end  = getBlockStartEnd<Functor>(last_start);

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
	Int walkIndexBw(
			Int start,
			Functor& walker,
			Int level_offet,
			Int level_size,
			Int cells_number_on_lower_level,
			Int cell_size
	) const
	{
		Int BranchingFactor = Functor::BranchingFactor;

		Int block_start_end     = getBlockStartEndBw<Functor>(start);

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
				Int level_size0 = getIndexCellsNumberFor<Functor>(level_size);
				Int last_start  = walkIndexBw(
						block_start_end / BranchingFactor,
						walker,
						level_offet - level_size0,
						level_size0,
						BranchingFactor,
						cell_size * BranchingFactor
				) - 1;

				Int last_start_end = getBlockStartEndBw<Functor>(last_start);

				return (walker.walkIndex(
						last_start + level_offet,
						last_start_end + level_offet,
						cell_size
				)
				- level_offet + 1) * cells_number_on_lower_level;
			}
		}
	}

protected:

	template <typename Fn>
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
					csize = ((csize % Fn::BranchingFactor) == 0) ?
								(csize / Fn::BranchingFactor) :
									(csize / Fn::BranchingFactor) + 1;
				}
				else {
					csize = ((csize % Fn::ValuesPerBranch) == 0) ?
								(csize / Fn::ValuesPerBranch) :
									(csize / Fn::ValuesPerBranch) + 1;
				}
				sum += csize;
			}
			return sum;
		}
	}

	template <typename Fn>
	static Int getMaxSize(Int block_size, Fn&& fn)
	{
		Int first       = 1;
		Int last        = fn.last(block_size);

		while (first < last - 1)
		{
			Int middle = (first + last) / 2;

			Int size = fn.getBlockSize(middle);
			if (size < block_size)
			{
				first = middle;
			}
			else if (size > block_size)
			{
				last = middle;
			}
			else {
				break;
			}
		}

		Int max_size;

		if (fn.getBlockSize(last) <= block_size)
		{
			max_size = last;
		}
		else if (fn.getBlockSize((first + last) / 2) <= block_size)
		{
			max_size = (first + last) / 2;
		}
		else {
			max_size = first;
		}

		Int max = fn.extend(max_size);

		if (fn.getIndexSize(max) <= fn.getIndexSize(max_size))
		{
			return max;
		}

		return max_size;
	}


	template <typename V>
	void dumpArray(std::ostream& out, Int count, function<V(Int)> fn) const
	{
	    Int columns;

	    switch (sizeof(V)) {
	    case 1: columns = 32; break;
	    case 2: columns = 16; break;
	    case 4: columns = 16; break;
	    default: columns = 8;
	    }

	    Int width = sizeof(V) * 2 + 1;

	    out<<endl;
	    Expand(out, 19 + width);
	    for (int c = 0; c < columns; c++)
	    {
	        out.width(width);
	        out<<hex<<c;
	    }
	    out<<endl;

	    for (Int c = 0; c < count; c+= columns)
	    {
	        Expand(out, 12);
	        out<<" ";
	        out.width(6);
	        out<<dec<<c<<" "<<hex;
	        out.width(6);
	        out<<c<<": ";

	        for (Int d = 0; d < columns && c + d < count; d++)
	        {
	            out<<hex;
	            out.width(width);
	            if (sizeof(V) == 1)
	            {
	                out<<(Int)fn(c + d);
	            }
	            else {
	                out<<fn(c + d);
	            }
	        }

	        out<<dec<<endl;
	    }
	}


	template <typename V>
	void dumpSymbols(ostream& out_, Int size_, Int bits_per_symbol, function<V(Int)> fn) const
	{
		Int columns;

		switch (bits_per_symbol)
		{
		case 1: columns = 100; break;
		case 2: columns = 100; break;
		case 4: columns = 100; break;
		default: columns = 50;
		}

		Int width = bits_per_symbol <= 4 ? 1 : 3;

		Int c = 0;

		do
		{
			out_<<endl;
			Expand(out_, 31 - width * 5 - (bits_per_symbol <= 4 ? 2 : 0));
			for (int c = 0; c < columns; c += 5)
			{
				out_.width(width*5);
				out_<<dec<<c;
			}
			out_<<endl;

			Int rows = 0;
			for (; c < size_ && rows < 10; c += columns, rows++)
			{
				Expand(out_, 12);
				out_<<" ";
				out_.width(6);
				out_<<dec<<c<<" "<<hex;
				out_.width(6);
				out_<<c<<": ";

				for (Int d = 0; d < columns && c + d < size_; d++)
				{
					out_<<hex;
					out_.width(width);
					out_<<fn(c + d);
				}

				out_<<dec<<endl;
			}
		} while (c < size_);
	}
};

}


#endif
