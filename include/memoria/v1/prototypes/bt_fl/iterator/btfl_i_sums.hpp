
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorStreamSumsName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::BranchNodeEntry                           BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;
    using DataSizesT  = typename Container::Types::DataSizesT;


    static constexpr int32_t DataStreams = Container::Types::DataStreams;
    static constexpr int32_t StructureStreamIdx = Container::Types::StructureStreamIdx;

private:


    template <typename IndexType, typename LeafPath>
    struct SumWalker {
        IndexType sum_{};
        int32_t block_;

        SumWalker(int32_t block): block_(block) {}

        template <typename NodeTypes>
        void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end)
        {
            using BranchNodeT = bt::BranchNode<NodeTypes>;

            using BranchPath = typename BranchNodeT::template BuildBranchPath<LeafPath>;

            int32_t branch_block = BranchNodeT::template translateLeafIndexToBranchIndex<LeafPath>(block_);

            auto substream = node->template substream<BranchPath>();

            sum_ += substream->sum(branch_block, end);
        }

        template <typename NodeTypes>
        void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end)
        {
            auto substream = node->template substream<LeafPath>();
            sum_ += substream->sum(block_, end);
        }
    };


//    struct SumsWalker {
//        DataSizesT ranks_;

//        template <typename NodeTypes>
//        void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end)
//        {
//        	  using BranchNodeT = bt::BranchNode<NodeTypes>;

//        	  using LeafPath = IntList<StructureStreamIdx, 1>;
//        	  using BranchPath = typename BranchNodeT::template BuildBranchPath<LeafPath>;

//        	  int32_t symbols_base = BranchNodeT::template translateLeafIndexToBranchIndex<LeafPath>(0);

//            auto sizes_substream = node->template substream<BranchPath>();

//            for (int32_t s = 0; s < DataStreams; s++) {
//            	ranks_[s] += sizes_substream->sum(s + symbols_base, end);
//            }
//        }

//        template <typename NodeTypes>
//        void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end)
//        {
//        }
//    };

public:

    template <typename SumT, typename LeafPath>
    SumT sum_up(int32_t block) const
    {
    	auto& self = this->self();
        SumWalker<SumT, LeafPath> fn(block);

        auto structure_idx = self.iter_local_pos();

        int32_t stream = ListHead<LeafPath>::Value;

        auto stream_idx = self.data_stream_idx(stream, structure_idx);

        self.ctr().ctr_walk_tree_up(self.iter_leaf(), stream_idx, fn);

        return fn.sum_;
    }



//    CtrSizeT ranks() const
//    {
//    	auto& self = this->self();
//    	RankWalker fn;

//    	auto idx = self.iter_local_pos();

//    	self.ctr().ctr_walk_tree_up(self.iter_leaf(), idx, fn);

//    	auto leaf_structure = self.leaf_structure();

//    	DataSizesT leaf_ranks;

//    	for (int32_t s = 0; s < DataStreams; s++) {
//    		leaf_ranks[s] = leaf_structure->rank(idx, s);
//    	}

//    	return fn.ranks_ + leaf_ranks;
//    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorStreamSumsName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}}
