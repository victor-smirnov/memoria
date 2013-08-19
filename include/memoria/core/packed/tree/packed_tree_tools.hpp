
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_TREE_TOOLS_HPP_
#define MEMORIA_CORE_PACKED_TREE_TOOLS_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>

namespace memoria {

template <typename T1, typename T2>
constexpr static T2 divUp(T1 v, T2 d) {
	return v / d + (v % d > 0);
}


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



template <Int BranchingFactor, Int ValuesPerBranch, typename LayoutValue = Short>
class PackedTreeTools {

private:

	static const Int LEVELS_MAX             = 32;

public:

	PackedTreeTools() {}


	template <typename Functor>
	static void reindex(Functor&& fn)
	{
		if (fn.tree().index_size() > 0)
		{
			auto layout = fn.tree().index_layout();

			Int index_tree_height = layout[0];

			Int base = 0;
			for (Int c = 1; c < index_tree_height; c++)
			{
				base += layout[c];
			}

			fn.buildFirstIndexLine(base, layout[index_tree_height]);

			for (Int c = index_tree_height; c > 1; c--, base -= layout[c])
			{
				Int level_size 	= layout[c];
				Int parent_base	= base - layout[c - 1];

				for (Int idx = 0; idx < level_size; idx += BranchingFactor)
				{
					Int next 	= idx + BranchingFactor;
					Int limit 	= next < level_size ? next : level_size;

					fn.processIndex(parent_base + idx / BranchingFactor, base + idx, base + limit);
				}
			}
		}
	}


	template <typename Functor, typename T>
	static void reindexBlock(Functor&& fn, const T* data, Int size)
	{
		if (fn.tree().index_size() > 0)
		{
			auto layout = fn.tree().index_layout();

			Int index_tree_height = layout[0];

			Int base = 0;
			for (Int c = 1; c < index_tree_height; c++)
			{
				base += layout[c];
			}

			fn.buildFirstIndexLine(base, layout[index_tree_height], data, size);

			for (Int c = index_tree_height; c > 1; c--, base -= layout[c])
			{
				Int level_size 	= layout[c];
				Int parent_base	= base - layout[c - 1];

				for (Int idx = 0; idx < level_size; idx += BranchingFactor)
				{
					Int next 	= idx + BranchingFactor;
					Int limit 	= next < level_size ? next : level_size;

					fn.processIndex(parent_base + idx / BranchingFactor, base + idx, base + limit);
				}
			}
		}
	}


	template <typename Functor>
	static void check(Functor&& fn)
	{
		if (fn.tree().index_size() > 0)
		{
			reindex(fn);
		}
		else {
			fn.checkData();
		}
	}

	template <typename Functor>
	static void reindex2(Functor&& fn)
	{
		if (fn.tree().index_size() > 0)
		{
			LayoutValue layout[16];

			Int capacity = fn.tree().raw_capacity();

			Int layout_size = compute_layout_size(capacity);

			buildIndexTreeLayout(layout, capacity, layout_size);

			Int index_tree_height = layout[0];

			Int base = 0;
			for (Int c = 1; c < index_tree_height; c++)
			{
				base += layout[c];
			}

			fn.buildFirstIndexLine(base, layout[index_tree_height]);

			for (Int c = index_tree_height; c > 1; c--, base -= layout[c])
			{
				Int level_size 	= layout[c];
				Int parent_base	= base - layout[c - 1];

				for (Int idx = 0; idx < level_size; idx += BranchingFactor)
				{
					Int next 	= idx + BranchingFactor;
					Int limit 	= next < level_size ? next : level_size;

					fn.processIndex(parent_base + idx / BranchingFactor, base + idx, base + limit);
				}
			}
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


	template <typename Walker>
	static Int find(Walker&& walker)
	{
		if (walker.has_index())
		{
			auto layout = walker.index_layout();

			Int index_tree_height = layout[0];

			Int start 	= 1;
			Int pos		= 0;

			for (Int c = 2, base = 1; c <= index_tree_height; c++)
			{
				Int level_size	= layout[c];
				Int limit 		= level_size < BranchingFactor ? level_size : BranchingFactor;
				Int end 		= start + limit;

				pos = walker.walkIndex(start, end);

				if (pos < end)
				{
					pos -= base;

					base += level_size;
					start = base + pos * BranchingFactor;
				}
				else {
					return walker.max();
				}
			}

			return walker.walkValues(pos);
		}
		else {
			return walker.walkValues(0);
		}
	}


	template <typename Walker>
	static Int find2(Walker&& walker)
	{
		if (walker.has_index())
		{
			LayoutValue layout[16];

			Int layout_size = compute_layout_size(walker.capacity());

			buildIndexTreeLayout(layout, walker.capacity(), layout_size);

			Int index_tree_height = layout[0];

			Int start 	= 1;
			Int pos		= 0;

			for (Int c = 2, base = 1; c <= index_tree_height; c++)
			{
				Int level_size	= layout[c];
				Int limit 		= level_size < BranchingFactor ? level_size : BranchingFactor;
				Int end 		= start + limit;

				pos = walker.walkIndex(start, end);

				if (pos < end)
				{
					pos -= base;

					base += level_size;
					start = base + pos * BranchingFactor;
				}
				else {
					return walker.max();
				}
			}

			return walker.walkValues(pos);
		}
		else {
			return walker.walkValues(0);
		}
	}



	template <typename Walker>
	static void walk(Int pos, Walker&& walker)
	{
		if (walker.has_index())
		{
			Int value_mask 	= ValuesPerBranch - 1;
			Int idx_mask 	= BranchingFactor - 1;

			Int idx = pos & value_mask;
			walker.walkValues(idx, pos);

			auto layout = walker.index_layout();

			Int index_tree_height = layout[0];

			Int base = 0;
			for (Int c = 1; c < index_tree_height; c++)
			{
				base += layout[c];
			}

			idx /= ValuesPerBranch;

			for (Int c = index_tree_height; c > 1; c--, base -= layout[c], idx /= BranchingFactor)
			{
				Int start = idx & idx_mask;

				walker.walkIndex(start + base, idx + base);
			}
		}
		else {
			walker.walkValues(0, pos);
		}
	}


	template <typename Walker>
	static void walk2(Int pos, Walker&& walker)
	{
		if (walker.has_index())
		{
			Int value_mask 	= ~(ValuesPerBranch - 1);
			Int idx_mask 	= ~(BranchingFactor - 1);

			Int idx = pos & value_mask;
			walker.walkValues(idx, pos);

			LayoutValue layout[16];

			Int layout_size = compute_layout_size(walker.capacity());

			buildIndexTreeLayout(layout, walker.capacity(), layout_size);

			Int index_tree_height = layout[0];

			Int base = 0;
			for (Int c = 1; c < index_tree_height; c++)
			{
				base += layout[c];
			}

			idx /= ValuesPerBranch;

			for (Int c = index_tree_height; c > 1; c--, base -= layout[c], idx /= BranchingFactor)
			{
				Int start = idx & idx_mask;

				walker.walkIndex(start + base, idx + base);
			}
		}
		else {
			walker.walkValues(0, pos);
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
					csize = divUp(csize, BranchingFactor);
				}
				else {
					csize = divUp(csize, ValuesPerBranch);
				}
				sum += csize;
			}
			return sum;
		}
	}

	static Int compute_layout_size(Int csize)
	{
		if (csize == 1)
		{
			return 2;
		}
		else {
			Int nlevels = 2;
			csize = divUp(csize, ValuesPerBranch);

			for (; csize > 1; nlevels++)
			{
				csize = divUp(csize, BranchingFactor);
			}

			return nlevels;
		}
	}



	template <typename T>
	static void buildIndexTreeLayout(T* buf, Int csize, Int layout_size)
	{
		if (layout_size > 0)
		{
			buf[0] = layout_size - 1;

			csize = buf[layout_size - 1] = divUp(csize, ValuesPerBranch);

			for (Int c = layout_size - 2; c > 0; c--)
			{
				csize = buf[c] = divUp(csize, BranchingFactor);
			}
		}
	}

};

template <typename MyType>
class ReindexFnBase {
public:
	static const Int Indexes        		= MyType::Indexes;

	typedef typename MyType:: IndexValue 	IndexValue;

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

		Int end 	= me_.index_size();
		Int start 	= 0;

		for (Int idx = 0; idx < Indexes; idx++)
		{
			for (Int c = start; c < end; c++)
			{
				indexes_[idx][c] = 0;
			}
		}
	}

	MyType& tree() {
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
	static const Int Indexes        			= MyType::Indexes;

	typedef typename MyType:: IndexValue 		IndexValue;

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

	const MyType& tree() const
	{
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

			MEMORIA_ASSERT(indexes_[idx][parent], ==, sum);
		}
	}
};


}


#endif
