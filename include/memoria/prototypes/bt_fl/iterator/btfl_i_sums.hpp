
// Copyright 2016 Victor Smirnov
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

#include <memoria/core/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>
#include <memoria/prototypes/bt/nodes/branch_node_so.hpp>
#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorStreamSumsName)

    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;
    using typename Base::Container;

    using DataSizesT  = typename Container::Types::DataSizesT;

    static constexpr int32_t DataStreams = Container::Types::DataStreams;
    static constexpr int32_t StructureStreamIdx = Container::Types::StructureStreamIdx;

private:


    template <typename IndexType, typename LeafPath>
    struct SumWalker {
        IndexType sum_{};
        int32_t block_;

        SumWalker(int32_t block): block_(block) {}

        template <typename CtrT, typename NodeTypes>
        VoidResult treeNode(const BranchNodeSO<CtrT, NodeTypes>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
        {
            using BranchNodeT = bt::BranchNode<NodeTypes>;

            using BranchPath = typename BranchNodeT::template BuildBranchPath<LeafPath>;

            int32_t branch_block = BranchNodeT::template translateLeafIndexToBranchIndex<LeafPath>(block_);

            auto substream = node.template substream<BranchPath>();

            sum_ += substream.sum(branch_block, end);

            return VoidResult::of();
        }

        template <typename CtrT, typename NodeTypes>
        VoidResult treeNode(const LeafNodeSO<CtrT, NodeTypes>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
        {
            auto substream = node.template substream<LeafPath>();
            sum_ += substream.sum(block_, end);
            return VoidResult::of();
        }
    };


public:

    template <typename SumT, typename LeafPath>
    Result<SumT> sum_up(int32_t block) const noexcept
    {
        using ResultT = Result<SumT>;

        auto& self = this->self();
        SumWalker<SumT, LeafPath> fn(block);

        auto structure_idx = self.iter_local_pos();

        int32_t stream = ListHead<LeafPath>::Value;

        auto stream_idx = self.data_stream_idx(stream, structure_idx);

        MEMORIA_TRY_VOID(self.ctr().ctr_walk_tree_up(self.iter_leaf(), stream_idx, fn));

        return ResultT::of(fn.sum_);
    }



MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorStreamSumsName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
