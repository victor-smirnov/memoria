
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl::IteratorStreamRankName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::BranchNodeEntry                           BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT 		= typename Container::Types::CtrSizeT;
    using DataSizesT  = typename Container::Types::DataSizesT;


    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    static constexpr Int DataStreams 				= Container::Types::DataStreams;
    static constexpr Int StructureStreamIdx = Container::Types::StructureStreamIdx;

private:


    struct RankWalker {
        CtrSizeT rank_ = 0;
        Int symbol_;
        RankWalker(Int symbol): symbol_(symbol) {}

        template <typename NodeTypes>
        void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
        {
            using BranchNodeT = bt::BranchNode<NodeTypes>;

            using LeafPath = IntList<StructureStreamIdx, 1>;
            using BranchPath = typename BranchNodeT::template BuildBranchPath<LeafPath>;

            Int symbols_base = BranchNodeT::template translateLeafIndexToBranchIndex<LeafPath>(0);

            auto sizes_substream = node->template substream<BranchPath>();

            rank_ += sizes_substream->sum(symbol_ + symbols_base, end);
        }

        template <typename NodeTypes>
        void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
        {
        }
    };


    struct RanksWalker {
        DataSizesT ranks_;

        template <typename NodeTypes>
        void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
        {
        	  using BranchNodeT = bt::BranchNode<NodeTypes>;

        	  using LeafPath = IntList<StructureStreamIdx, 1>;
        	  using BranchPath = typename BranchNodeT::template BuildBranchPath<LeafPath>;

        	  Int symbols_base = BranchNodeT::template translateLeafIndexToBranchIndex<LeafPath>(0);

            auto sizes_substream = node->template substream<BranchPath>();

            for (Int s = 0; s < DataStreams; s++) {
            	ranks_[s] += sizes_substream->sum(s + symbols_base, end);
            }
        }

        template <typename NodeTypes>
        void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
        {
        }
    };

public:

    CtrSizeT rank(Int stream) const
    {
    	auto& self = this->self();
    	RankWalker fn(stream);

    	auto idx = self.idx();

    	self.ctr().walkUp(self.leaf(), idx, fn);

    	auto leaf_rank = self.leaf_structure()->rank(idx, stream);

    	return fn.rank_ + leaf_rank;
    }



    CtrSizeT ranks() const
    {
    	auto& self = this->self();
    	RankWalker fn;

    	auto idx = self.idx();

    	self.ctr().walkUp(self.leaf(), idx, fn);

    	auto leaf_structure = self.leaf_structure();

    	DataSizesT leaf_ranks;

    	for (Int s = 0; s < DataStreams; s++) {
    		leaf_ranks[s] = leaf_structure->rank(idx, s);
    	}

    	return fn.ranks_ + leaf_ranks;
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorStreamRankName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}}