
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

template <typename IndexValueT, int32_t kBranchingFactor, int32_t kValuesPerBranch, int32_t SegmentsPerBlock, typename MetadataT>
class PkdFQTreeBaseBase: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr uint32_t VERSION = 1;

    using IndexValue    = IndexValueT;
    using Metadata      = MetadataT;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;

    static const int32_t BranchingFactor        = kBranchingFactor;
    static const int32_t ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = true;

    static constexpr int32_t ValuesPerBranchMask    = ValuesPerBranch - 1;
    static constexpr int32_t BranchingFactorMask    = BranchingFactor - 1;

    static constexpr int32_t ValuesPerBranchLog2    = Log2(ValuesPerBranch) - 1;
    static constexpr int32_t BranchingFactorLog2    = Log2(BranchingFactor) - 1;

    static constexpr int32_t METADATA = 0;


    struct TreeLayout {
        int32_t level_starts[8];
        int32_t level_sizes[8];
        int32_t levels_max = 0;
        int32_t index_size = 0;
    };

    template <typename IndexT>
    struct IndexedTreeLayout: TreeLayout {
        const IndexT* indexes;
    };


public:

    PkdFQTreeBaseBase() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<int32_t, kBranchingFactor>,
                ConstValue<int32_t, kValuesPerBranch>,
                decltype(Metadata::size_),
                decltype(Metadata::index_size_),
                IndexValue
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


    template <typename IndexT, int32_t IndexNum>
    IndexT* index(int32_t block) {
        return this->template get<IndexT>(block * SegmentsPerBlock + 1 + IndexNum);
    }

    template <typename IndexT, int32_t IndexNum>
    const IndexT* index(int32_t block) const {
        return this->template get<IndexT>(block * SegmentsPerBlock + 1 + IndexNum);
    }


    const int32_t& size() const {
        return metadata()->size();
    }

    int32_t& size() {
        return metadata()->size();
    }

    int32_t index_size() const {
        return metadata()->index_size();
    }

    int32_t max_size() const {
        return metadata()->max_size();
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


    int32_t compute_tree_layout(const Metadata* meta, TreeLayout& layout) const {
        return compute_tree_layout(meta->max_size(), layout);
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


    template <typename IndexT, int32_t IndexNum>
    auto sum_index(IndexedTreeLayout<IndexT>& layout, int32_t block, int32_t start, int32_t end) const
    {
        layout.indexes = this->template index<IndexT, IndexNum>(block);

        IndexT sum = 0;

        this->sum_index(layout, sum, start, end, layout.levels_max);

        return sum;
    }


    template <typename IndexT>
    void sum_index(const IndexedTreeLayout<IndexT>& layout, IndexT& sum, int32_t start, int32_t end, int32_t level) const
    {
        int32_t level_start = layout.level_starts[level];

        int32_t branch_end = (start | BranchingFactorMask) + 1;
        int32_t branch_start = end & ~BranchingFactorMask;

        if (end <= branch_end || branch_start == branch_end)
        {
            for (int32_t c = start + level_start; c < end + level_start; c++)
            {
                sum += layout.indexes[c];
            }
        }
        else {
            for (int32_t c = start + level_start; c < branch_end + level_start; c++)
            {
                sum += layout.indexes[c];
            }

            sum_index(
                    layout,
                    sum,
                    branch_end >> BranchingFactorLog2,
                    branch_start >> BranchingFactorLog2,
                    level - 1
            );

            for (int32_t c = branch_start + level_start; c < end + level_start; c++)
            {
                sum += layout.indexes[c];
            }
        }
    }


    template <typename IndexT, typename Walker>
    int32_t walk_index_fw(const IndexedTreeLayout<IndexT>& data, int32_t start, int32_t level, Walker&& walker) const
    {
        int32_t level_start = data.level_starts[level];
        int32_t level_size = data.level_sizes[level];

        int32_t branch_end = (start | BranchingFactorMask) + 1;

        int32_t branch_limit;

        if (branch_end > level_size) {
            branch_limit = level_size;
        }
        else {
            branch_limit = branch_end;
        }

        for (int32_t c = level_start + start; c < branch_limit + level_start; c++)
        {
            if (walker.compare(data.indexes[c]))
            {
                if (level < data.levels_max)
                {
                    return walk_index_fw(
                            data,
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
                walker.next();
            }
        }

        if (level > 0)
        {
            return walk_index_fw(
                    data,
                    branch_end >> BranchingFactorLog2,
                    level - 1,
                    std::forward<Walker>(walker)
            );
        }
        else {
            return -1;
        }
    }


    template <typename IndexT, typename Walker>
    int32_t walk_index_bw(const IndexedTreeLayout<IndexT>& data, int32_t start, int32_t level, Walker&& walker) const
    {
        int32_t level_start = data.level_starts[level];
        int32_t level_size  = data.level_sizes[level];

        if (start >= 0)
        {
            int32_t branch_end = (start & ~BranchingFactorMask) - 1;

            if (start >= level_size) {
                start = level_size - 1;
            }

            for (int32_t c = level_start + start; c > branch_end + level_start; c--)
            {
                if (walker.compare(data.indexes[c]))
                {
                    if (level < data.levels_max)
                    {
                        return walk_index_bw(
                                data,
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
                    walker.next();
                }
            }

            if (level > 0)
            {
                return walk_index_bw(
                        data,
                        branch_end >> BranchingFactorLog2,
                        level - 1,
                        std::forward<Walker>(walker)
                );
            }
            else {
                return -1;
            }
        }
        else {
            return -1;
        }
    }



    template <typename IndexT, typename Walker>
    int32_t find_index(const IndexedTreeLayout<IndexT>& data, Walker&& walker) const
    {
        int32_t branch_start = 0;

        for (int32_t level = 1; level <= data.levels_max; level++)
        {
            int32_t level_start = data.level_starts[level];

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
                    walker.next();
                }
            }

            return -1;

            next_level:;
        }

        return -1;
    }
};

}
