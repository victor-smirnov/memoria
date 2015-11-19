
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_QUICK_TREE_BASE_BASE_HPP_
#define MEMORIA_CORE_PACKED_VLE_QUICK_TREE_BASE_BASE_HPP_

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/packed/tree/packed_tree_walkers.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/static_array.hpp>


#include <type_traits>

namespace memoria {



template <typename IndexValueT, Int kBranchingFactor, Int kValuesPerBranch, Int SegmentsPerBlock, typename MetadataT>
class PkdVQTreeBaseBase: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr UInt VERSION = 1;

    using IndexValue 	= IndexValueT;
    using Metadata		= MetadataT;

    static const Int BranchingFactor        = kBranchingFactor;
    static const Int ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = true;

    static constexpr Int ValuesPerBranchMask 	= ValuesPerBranch - 1;
    static constexpr Int BranchingFactorMask 	= BranchingFactor - 1;

    static constexpr Int ValuesPerBranchLog2 	= Log2(ValuesPerBranch) - 1;
    static constexpr Int BranchingFactorLog2 	= Log2(BranchingFactor) - 1;



    enum {
    	VALUE_INDEX, SIZE_INDEX, OFFSETS, VALUES
    };


    static constexpr Int METADATA 	 	= 0;
    static constexpr Int DATA_SIZES 	= 1;
    static constexpr Int BlocksStart 	= 2;

    struct TreeLayout {
    	Int level_starts[8];
    	Int level_sizes[8];
    	Int levels_max = 0;
    	Int index_size = 0;

    	const IndexValueT* indexes;
    	const Int* valaue_block_size_prefix;

//    	IndexValueT value_sum 		= 0;
//    	Int size_prefix_sum   		= 0;
//    	Int size_prefix_idx_sum   	= 0;
    };




public:

    PkdVQTreeBaseBase() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				ConstValue<Int, kBranchingFactor>,
				ConstValue<Int, kValuesPerBranch>,
				IndexValue
    >;


    static Int index_size(Int capacity)
    {
    	TreeLayout layout;
    	compute_tree_layout(capacity, layout);
    	return layout.index_size;
    }

    Metadata* metadata() {
    	return this->template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const {
    	return this->template get<Metadata>(METADATA);
    }

    const Int* data_sizes() const {
    	return this->template get<Int>(DATA_SIZES);
    }

    Int* data_sizes() {
    	return this->template get<Int>(DATA_SIZES);
    }

    Int& data_size(Int block) {
    	return data_sizes()[block];
    }

    const Int& data_size(Int block) const {
    	return data_sizes()[block];
    }


    IndexValueT* value_index(Int block) {
    	return this->template get<IndexValueT>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart);
    }


    const IndexValueT* value_index(Int block) const {
    	return this->template get<IndexValueT>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart);
    }

    Int* size_index(Int block) {
    	return this->template get<Int>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart);
    }

    const Int* size_index(Int block) const {
    	return this->template get<Int>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart);
    }



    const Int& size() const {
    	return metadata()->size();
    }

    Int& size() {
    	return metadata()->size();
    }

    Int index_size() const {
    	return metadata()->index_size();
    }

protected:

    static constexpr Int divUpV(Int value) {
    	return (value >> ValuesPerBranchLog2) + ((value & ValuesPerBranchMask) ? 1 : 0);
    }

    static constexpr Int divUpI(Int value) {
    	return (value >> BranchingFactorLog2) + ((value & BranchingFactorMask) ? 1 : 0);
    }

    template <Int Divisor>
    static constexpr Int divUp(Int value, Int divisor) {
    	return (value / Divisor) + ((value % Divisor) ? 1 : 0);
    }


//    Int compute_tree_layout(const Metadata* meta, TreeLayout& layout) const {
//    	return compute_tree_layout(meta->max_size(), layout);
//    }


    static TreeLayout compute_tree_layout(Int size)
    {
    	TreeLayout layout;
    	compute_tree_layout(size, layout);
    	return layout;
    }

    static Int compute_tree_layout(Int size, TreeLayout& layout)
    {
    	if (size <= ValuesPerBranch)
    	{
    		layout.levels_max = -1;
    		layout.index_size = 0;

    		return 0;
    	}
    	else {
    		Int level = 0;

    		layout.level_sizes[level] = divUpV(size);
    		level++;

    		while((layout.level_sizes[level] = divUpI(layout.level_sizes[level - 1])) > 1)
    		{
    			level++;
    		}

    		level++;

    		for (int c = 0; c < level / 2; c++)
    		{
    			auto tmp = layout.level_sizes[c];
    			layout.level_sizes[c] = layout.level_sizes[level - c - 1];
    			layout.level_sizes[level - c - 1] = tmp;
    		}

    		Int level_start = 0;

    		for (int c = 0; c < level; c++)
    		{
    			layout.level_starts[c] = level_start;
    			level_start += layout.level_sizes[c];
    		}

    		layout.index_size = level_start;
    		layout.levels_max = level - 1;

    		return level;
    	}
    }


    auto sum_index(TreeLayout& layout, Int block, Int start, Int end) const
    {
    	layout.indexes = this->value_index(block);
    	layout.valaue_block_size_prefix = this->size_index(block);

    	IndexValueT sum = 0;
    	Int size_prefix_sum = 0;

    	this->sum_index(layout, sum, size_prefix_sum, start, end, layout.levels_max);

    	return sum;
    }



    void sum_index(const TreeLayout& layout, Int start, Int end, Int level) const
    {
    	Int level_start = layout.level_starts[level];

    	Int branch_end = (start | BranchingFactorMask) + 1;
    	Int branch_start = end & ~BranchingFactorMask;

    	if (end <= branch_end || branch_start == branch_end)
    	{
    		for (Int c = start + level_start; c < end + level_start; c++)
    		{
    			layout.sum += layout.indexes[c];
    			layout.size_prefix_sum += layout.valaue_block_size_prefix[c];
    		}
    	}
    	else {
    		for (Int c = start + level_start; c < branch_end + level_start; c++)
    		{
    			layout.sum += layout.indexes[c];
    			layout.size_prefix_sum += layout.valaue_block_size_prefix[c];
    		}

    		sum_index(
    				layout,
    				branch_end >> BranchingFactorLog2,
					branch_start >> BranchingFactorLog2,
					level - 1
    		);

    		for (Int c = branch_start + level_start; c < end + level_start; c++)
    		{
    			layout.sum += layout.indexes[c];
    			layout.size_prefix_sum += layout.valaue_block_size_prefix[c];
    		}
    	}
    }

    struct WalkerState {
    	Int size_sum = 0;
    };

    template <typename Walker>
    Int walk_index_fw(const TreeLayout& data, WalkerState& state, Int start, Int level, Walker&& walker) const
    {
    	Int level_start = data.level_starts[level];

    	Int branch_end = (start | BranchingFactorMask) + 1;

    	for (Int c = level_start + start; c < branch_end + level_start; c++)
    	{
    		if (walker.compare(data.indexes[c]))
    		{
    			if (level < data.levels_max)
    			{
    				return walk_index_fw(
    						data,
							state,
							(c - level_start) << BranchingFactorLog2,
							level + 1,
							std::forward<Walker>(walker)
    				);
    			}
    			else {
    				return c - level_start;
    			}
    		}
    		else {
    			state.size_sum += data.valaue_block_size_prefix[c];
    			walker.next();
    		}
    	}

    	if (level > 0)
    	{
    		return walk_index_fw(
    				data,
					state,
    				branch_end >> BranchingFactorLog2,
					level - 1,
					std::forward<Walker>(walker)
    		);
    	}
    	else {
    		return -1;
    	}
    }


    template <typename Walker>
    Int walk_index_bw(const TreeLayout& data, WalkerState& state, Int start, Int level, Walker&& walker) const
    {
    	Int level_start = data.level_starts[level];
    	Int level_size  = data.level_sizes[level];

    	Int branch_end = (start & ~BranchingFactorMask) - 1;

    	if (start >= level_size) {
    		start = level_size - 1;
    	}

    	for (Int c = level_start + start; c > branch_end + level_start; c--)
    	{
    		if (walker.compare(data.indexes[c]))
    		{
    			if (level < data.levels_max)
    			{
    				return walk_index_bw(
    						data,
							state,
							((c - level_start + 1) << BranchingFactorLog2) - 1,
							level + 1,
							std::forward<Walker>(walker)
    				);
    			}
    			else {
    				return c - level_start;
    			}
    		}
    		else {
    			state.size_sum -= data.valaue_block_size_prefix[c];
    			walker.next();
    		}
    	}

    	if (level > 0)
    	{
    		return walk_index_bw(
    				data,
					state,
					branch_end >> BranchingFactorLog2,
					level - 1,
					std::forward<Walker>(walker)
    		);
    	}
    	else {
    		return -1;
    	}
    }



    template <typename Walker>
    Int find_index(const TreeLayout& data, WalkerState& state, Walker&& walker) const
    {
    	Int branch_start = 0;

    	for (Int level = 1; level <= data.levels_max; level++)
    	{
    		Int level_start = data.level_starts[level];

    		for (int c = level_start + branch_start; c < level_start + data.level_sizes[level]; c++)
    		{
    			if (walker.compare(data.indexes[c]))
    			{
    				if (level < data.levels_max)
    				{
    					branch_start = (c - level_start) << BranchingFactorLog2;
    					goto next_level;
    				}
    				else {
    					return (c - level_start);
    				}
    			}
    			else {
    				state.size_sum += data.valaue_block_size_prefix[c];
    				walker.next();
    			}
    		}

    		return -1;

    		next_level:;
    	}

    	return -1;
    }


    struct LocateResult {
    	Int idx = 0;
    	Int index_cnt = 0;
    	IndexValue value_sum = 0;

    	LocateResult(Int idx_, Int index_cnt_ = 0, Int value_sum_ = 0) :
    		idx(idx_), index_cnt(index_cnt_), value_sum(value_sum_)
    	{}

    	LocateResult() {}

    	Int local_cnt() const {return idx - index_cnt;}
    };


    LocateResult locate_index(TreeLayout& data, Int idx) const
    {
    	Int branch_start = 0;

    	Int sum = 0;

    	for (Int level = 1; level <= data.levels_max; level++)
    	{
    		Int level_start = data.level_starts[level];

    		for (int c = level_start + branch_start; c < level_start + data.level_sizes[level]; c++)
    		{
    			if (sum + data.valaue_block_size_prefix[c] > idx)
    			{
    				if (level < data.levels_max)
    				{
    					branch_start = (c - level_start) << BranchingFactorLog2;
    					goto next_level;
    				}
    				else {
    					return LocateResult(c - level_start, sum);
    				}
    			}
    			else {
    				sum += data.valaue_block_size_prefix[c];
    			}
    		}

    		return LocateResult(-1, sum);

    		next_level:;
    	}

    	return LocateResult(-1, sum);
    }


    LocateResult locate_index_with_sum(const TreeLayout& data, Int idx) const
    {
    	Int branch_start = 0;

    	Int sum = 0;
    	IndexValueT value_sum = 0;

    	for (Int level = 1; level <= data.levels_max; level++)
    	{
    		Int level_start = data.level_starts[level];

    		for (int c = level_start + branch_start; c < level_start + data.level_sizes[level]; c++)
    		{
    			if (sum + data.valaue_block_size_prefix[c] > idx)
    			{
    				if (level < data.levels_max)
    				{
    					branch_start = (c - level_start) << BranchingFactorLog2;
    					goto next_level;
    				}
    				else {
    					return LocateResult(c - level_start, sum, value_sum);
    				}
    			}
    			else {
    				sum += data.valaue_block_size_prefix[c];
    				value_sum += data.indexes[c];
    			}
    		}

    		return LocateResult(-1, sum, value_sum);

    		next_level:;
    	}

    	return LocateResult(-1, sum, value_sum);
    }

};

}


#endif
