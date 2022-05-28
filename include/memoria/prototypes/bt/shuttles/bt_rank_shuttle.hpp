
// Copyright 2022 Victor Smirnov
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

#include <memoria/prototypes/bt/shuttles/bt_shuttle_base.hpp>

#include <memoria/core/types/algo.hpp>

namespace memoria::bt {




template <typename Types, typename LeafPath>
class RankShuttle: public UptreeShuttle<Types> {
    using Base = UptreeShuttle<Types>;
protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using CtrSizeT = typename Types::CtrSizeT;

    CtrSizeT leaf_end_;
    size_t symbol_;
    SeqOpType op_type_;
    CtrSizeT rank_{};

public:
    RankShuttle(CtrSizeT leaf_end, size_t symbol, SeqOpType op_type):
        leaf_end_(leaf_end),
        symbol_(symbol),
        op_type_(op_type)
    {}


    virtual void treeNode(const BranchNodeTypeSO& node, size_t end)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t symbol = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(symbol_);

        auto ss = node.template substream<BranchPath>();
        rank_ += ss.sum_for_rank(0, end, symbol, op_type_);
    }

    virtual void treeNode(const LeafNodeTypeSO& node)
    {
        auto ss = node.template substream<LeafPath>();
        rank_ += ss.rank(leaf_end_, symbol_, op_type_);
    }

    const CtrSizeT& rank(){return rank_;}
};




}
