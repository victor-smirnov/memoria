
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>

#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/dump.hpp>
#include <memoria/v1/core/tools/reflection.hpp>
#include <memoria/v1/core/tools/accessors.hpp>
#include <memoria/v1/core/tools/static_array.hpp>


#include <type_traits>

namespace memoria {



template <Int kBranchingFactor, Int kValuesPerBranch, Int SegmentsPerBlock, typename MetadataT>
class PkdVLEArrayBaseBase: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr UInt VERSION = 1;

    using Metadata      = MetadataT;

    static const Int BranchingFactor        = kBranchingFactor;
    static const Int ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = true;

    static constexpr Int ValuesPerBranchMask    = ValuesPerBranch - 1;
    static constexpr Int BranchingFactorMask    = BranchingFactor - 1;

    static constexpr Int ValuesPerBranchLog2    = Log2(ValuesPerBranch) - 1;
    static constexpr Int BranchingFactorLog2    = Log2(BranchingFactor) - 1;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;

    enum {
        SIZE_INDEX, OFFSETS, VALUES
    };


    static constexpr Int METADATA       = 0;
    static constexpr Int DATA_SIZES     = 1;
    static constexpr Int BlocksStart    = 1;

    struct TreeLayout {
        Int level_starts[8];
        Int level_sizes[8];
        Int levels_max = 0;
        Int index_size = 0;

        const Int* valaue_block_size_prefix;
    };


public:

    PkdVLEArrayBaseBase() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                ConstValue<Int, kBranchingFactor>,
                ConstValue<Int, kValuesPerBranch>
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

    const Int& data_size(Int block) const {
        return metadata()->data_size(block);
    }

    Int& data_size(Int block) {
        return metadata()->data_size(block);
    }

    const Int& max_data_size(Int block) const {
        return metadata()->max_data_size(block);
    }

    Int& max_data_size(Int block) {
        return metadata()->max_data_size(block);
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








    struct LocateResult {
        Int idx = 0;
        Int index_cnt = 0;

        LocateResult(Int idx_, Int index_cnt_ = 0) :
            idx(idx_), index_cnt(index_cnt_)
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



};

}
