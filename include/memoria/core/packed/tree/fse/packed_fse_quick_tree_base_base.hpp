
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

template <typename IndexValueT, size_t kBranchingFactor, size_t kValuesPerBranch, size_t SegmentsPerBlock, typename MetadataT>
class PkdFQTreeBaseBase: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr uint32_t VERSION = 1;

    using IndexValue    = IndexValueT;
    using Metadata      = MetadataT;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;

    static const size_t BranchingFactor        = kBranchingFactor;
    static const size_t ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = true;

    static constexpr size_t ValuesPerBranchMask    = ValuesPerBranch - 1;
    static constexpr size_t BranchingFactorMask    = BranchingFactor - 1;

    static constexpr size_t ValuesPerBranchLog2    = Log2(ValuesPerBranch) - 1;
    static constexpr size_t BranchingFactorLog2    = Log2(BranchingFactor) - 1;

    static constexpr size_t METADATA = 0;


    struct TreeLayout {
        size_t level_starts[8];
        size_t level_sizes[8];
        size_t levels_max = 0;
        size_t index_size = 0;
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
                ConstValue<size_t, kBranchingFactor>,
                ConstValue<size_t, kValuesPerBranch>,
                decltype(Metadata::size_),
                decltype(Metadata::index_size_),
                IndexValue
    >;


    static size_t index_size(size_t capacity)
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


    template <typename IndexT, size_t IndexNum>
    IndexT* index(size_t block) {
        return this->template get<IndexT>(block * SegmentsPerBlock + 1 + IndexNum);
    }

    template <typename IndexT, size_t IndexNum>
    const IndexT* index(size_t block) const {
        return this->template get<IndexT>(block * SegmentsPerBlock + 1 + IndexNum);
    }


    const psize_t& size() const {
        return metadata()->size();
    }

    psize_t& size() {
        return metadata()->size();
    }

    size_t index_size() const {
        return metadata()->index_size();
    }

    size_t max_size() const {
        return metadata()->max_size();
    }



protected:

    static constexpr size_t div_upV(size_t value) {
        return (value >> ValuesPerBranchLog2) + ((value & ValuesPerBranchMask) ? 1 : 0);
    }

    static constexpr size_t div_upI(size_t value) {
        return (value >> BranchingFactorLog2) + ((value & BranchingFactorMask) ? 1 : 0);
    }

    template <size_t Divisor>
    static constexpr size_t div_up(size_t value, size_t divisor) {
        return (value / Divisor) + ((value % Divisor) ? 1 : 0);
    }


    size_t compute_tree_layout(const Metadata* meta, TreeLayout& layout) const {
        return compute_tree_layout(meta->max_size(), layout);
    }


    static size_t compute_tree_layout(size_t size, TreeLayout& layout)
    {
        if (size <= ValuesPerBranch)
        {
            layout.levels_max = -1;
            layout.index_size = 0;

            return 0;
        }
        else {
            size_t level = 0;

            layout.level_sizes[level] = div_upV(size);
            level++;

            while((layout.level_sizes[level] = div_upI(layout.level_sizes[level - 1])) > 1)
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

            size_t level_start = 0;

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


    template <typename IndexT, size_t IndexNum>
    auto sum_index(IndexedTreeLayout<IndexT>& layout, size_t block, size_t start, size_t end) const
    {
        layout.indexes = this->template index<IndexT, IndexNum>(block);

        IndexT sum = 0;

        this->sum_index(layout, sum, start, end, layout.levels_max);

        return sum;
    }


    template <typename IndexT>
    void sum_index(const IndexedTreeLayout<IndexT>& layout, IndexT& sum, size_t start, size_t end, size_t level) const
    {
        size_t level_start = layout.level_starts[level];

        size_t branch_end = (start | BranchingFactorMask) + 1;
        size_t branch_start = end & ~BranchingFactorMask;

        if (end <= branch_end || branch_start == branch_end)
        {
            for (size_t c = start + level_start; c < end + level_start; c++)
            {
                sum += layout.indexes[c];
            }
        }
        else {
            for (size_t c = start + level_start; c < branch_end + level_start; c++)
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

            for (size_t c = branch_start + level_start; c < end + level_start; c++)
            {
                sum += layout.indexes[c];
            }
        }
    }


    template <typename IndexT, typename Walker>
    size_t walk_index_fw(const IndexedTreeLayout<IndexT>& data, size_t start, size_t level, Walker&& walker) const
    {
        size_t level_start = data.level_starts[level];
        size_t level_size = data.level_sizes[level];

        size_t branch_end = (start | BranchingFactorMask) + 1;

        size_t branch_limit;

        if (branch_end > level_size) {
            branch_limit = level_size;
        }
        else {
            branch_limit = branch_end;
        }

        for (size_t c = level_start + start; c < branch_limit + level_start; c++)
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
    size_t walk_index_bw(const IndexedTreeLayout<IndexT>& data, size_t start, size_t level, Walker&& walker) const
    {
        size_t level_start = data.level_starts[level];
        size_t level_size  = data.level_sizes[level];

        if (start >= 0)
        {
            size_t branch_end = (start & ~BranchingFactorMask) - 1;

            if (start >= level_size) {
                start = level_size - 1;
            }

            for (size_t c = level_start + start; c > branch_end + level_start; c--)
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
    size_t find_index(const IndexedTreeLayout<IndexT>& data, Walker&& walker) const
    {
        size_t branch_start = 0;

        for (size_t level = 1; level <= data.levels_max; level++)
        {
            size_t level_start = data.level_starts[level];

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
