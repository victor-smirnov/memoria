
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

#include <memoria/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/prototypes/bt/nodes/branch_node.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorStreamRankName)

    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;
    using typename Base::Container;

    using DataSizesT  = typename Container::Types::DataSizesT;

    static constexpr int32_t DataStreams 				= Container::Types::DataStreams;
    static constexpr int32_t StructureStreamIdx = Container::Types::StructureStreamIdx;

private:


    struct RankWalker {
        CtrSizeT rank_{};
        int32_t symbol_;
        RankWalker(int32_t symbol): symbol_(symbol) {}

        template <typename CtrT, typename NodeTypes>
        VoidResult treeNode(const BranchNodeSO<CtrT, NodeTypes>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
        {
            using BranchNodeT = bt::BranchNode<NodeTypes>;

            using LeafPath = IntList<StructureStreamIdx, 1>;
            using BranchPath = typename BranchNodeT::template BuildBranchPath<LeafPath>;

            int32_t symbols_base = BranchNodeT::template translateLeafIndexToBranchIndex<LeafPath>(0);

            auto sizes_substream = node.template substream<BranchPath>();

            rank_ += sizes_substream.sum(symbol_ + symbols_base, end);

            return VoidResult::of();
        }

        template <typename CtrT, typename NodeTypes>
        VoidResult treeNode(const LeafNodeSO<CtrT, NodeTypes>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
        {
            return VoidResult::of();
        }
    };


    struct RanksWalker {
        DataSizesT ranks_;

        template <typename CtrT, typename NodeTypes>
        VoidResult treeNode(const BranchNodeSO<CtrT, NodeTypes>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
        {
            using BranchNodeT = bt::BranchNode<NodeTypes>;

            using LeafPath = IntList<StructureStreamIdx, 1>;
            using BranchPath = typename BranchNodeT::template BuildBranchPath<LeafPath>;

            int32_t symbols_base = BranchNodeT::template translateLeafIndexToBranchIndex<LeafPath>(0);

            auto sizes_substream = node.template substream<BranchPath>();

            for (int32_t s = 0; s < DataStreams; s++) {
                ranks_[s] += sizes_substream.sum(s + symbols_base, end);
            }

            return VoidResult::of();
        }

        template <typename NodeTypes>
        VoidResult treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end) noexcept
        {
            return VoidResult::of();
        }
    };

public:

    Result<CtrSizeT> iter_rank(int32_t stream) const noexcept
    {
        using ResultT = Result<CtrSizeT>;

        auto& self = this->self();
    	RankWalker fn(stream);

    	auto idx = self.iter_local_pos();

        MEMORIA_TRY_VOID(self.ctr().ctr_walk_tree_up(self.iter_leaf(), idx, fn));

    	auto leaf_rank = self.leaf_structure()->rank(idx, stream);

        return ResultT::of(fn.rank_ + leaf_rank);
    }


    Result<CtrSizeT> iter_ranks() const noexcept
    {
        using ResultT = Result<CtrSizeT>;

    	auto& self = this->self();
    	RankWalker fn;

    	auto idx = self.iter_local_pos();

        MEMORIA_TRY_VOID(self.ctr().ctr_walk_tree_up(self.iter_leaf(), idx, fn));

    	auto leaf_structure = self.leaf_structure();

    	DataSizesT leaf_ranks;

    	for (int32_t s = 0; s < DataStreams; s++) {
    		leaf_ranks[s] = leaf_structure->rank(idx, s);
    	}

        return ResultT::of(fn.ranks_ + leaf_ranks);
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorStreamRankName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
