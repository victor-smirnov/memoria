
// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/static_array.hpp>


#include <type_traits>

namespace memoria {

template <int32_t kBranchingFactor, int32_t kValuesPerBranch, int32_t SegmentsPerBlock, typename MetadataT>
class PkdVLEArrayBaseBase: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr uint32_t VERSION = 1;

    using Metadata      = MetadataT;

    static const int32_t BranchingFactor        = kBranchingFactor;
    static const int32_t ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = true;

    static constexpr int32_t ValuesPerBranchMask    = ValuesPerBranch - 1;
    static constexpr int32_t BranchingFactorMask    = BranchingFactor - 1;

    static constexpr int32_t ValuesPerBranchLog2    = Log2(ValuesPerBranch) - 1;
    static constexpr int32_t BranchingFactorLog2    = Log2(BranchingFactor) - 1;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;

    enum {
        SIZE_INDEX, OFFSETS, VALUES
    };


    static constexpr int32_t METADATA       = 0;
    static constexpr int32_t DATA_SIZES     = 1;
    static constexpr int32_t BlocksStart    = 1;

    struct TreeLayout {
        int32_t level_starts[8];
        int32_t level_sizes[8];
        int32_t levels_max = 0;
        int32_t index_size = 0;

        const int32_t* valaue_block_size_prefix;
    };


public:

    PkdVLEArrayBaseBase() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<int32_t, kBranchingFactor>,
                ConstValue<int32_t, kValuesPerBranch>
    >;


    static int32_t index_size(int32_t capacity)
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

    int32_t* size_index(int32_t block) {
        return this->template get<int32_t>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart);
    }

    const int32_t* size_index(int32_t block) const {
        return this->template get<int32_t>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart);
    }



    const int32_t& size() const {
        return metadata()->size();
    }

    int32_t& size() {
        return metadata()->size();
    }

    const int32_t& data_size(int32_t block) const {
        return metadata()->data_size(block);
    }

    int32_t& data_size(int32_t block) {
        return metadata()->data_size(block);
    }

    const int32_t& max_data_size(int32_t block) const {
        return metadata()->max_data_size(block);
    }

    int32_t& max_data_size(int32_t block) {
        return metadata()->max_data_size(block);
    }

protected:

    static constexpr int32_t divUpV(int32_t value) {
        return (value >> ValuesPerBranchLog2) + ((value & ValuesPerBranchMask) ? 1 : 0);
    }

    static constexpr int32_t divUpI(int32_t value) {
        return (value >> BranchingFactorLog2) + ((value & BranchingFactorMask) ? 1 : 0);
    }

    template <int32_t Divisor>
    static constexpr int32_t divUp(int32_t value, int32_t divisor) {
        return (value / Divisor) + ((value % Divisor) ? 1 : 0);
    }


    static TreeLayout compute_tree_layout(int32_t size)
    {
        TreeLayout layout;
        compute_tree_layout(size, layout);
        return layout;
    }

    static int32_t compute_tree_layout(int32_t size, TreeLayout& layout)
    {
        if (size <= ValuesPerBranch)
        {
            layout.levels_max = -1;
            layout.index_size = 0;

            return 0;
        }
        else {
            int32_t level = 0;

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

            int32_t level_start = 0;

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
        int32_t idx = 0;
        int32_t index_cnt = 0;

        LocateResult(int32_t idx_, int32_t index_cnt_ = 0) :
            idx(idx_), index_cnt(index_cnt_)
        {}

        LocateResult() {}

        int32_t local_cnt() const {return idx - index_cnt;}
    };


    LocateResult locate_index(TreeLayout& data, int32_t idx) const
    {
        int32_t branch_start = 0;

        int32_t sum = 0;

        for (int32_t level = 1; level <= data.levels_max; level++)
        {
            int32_t level_start = data.level_starts[level];

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
