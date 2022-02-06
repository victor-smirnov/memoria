
// Copyright 2013 Victor Smirnov
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

#include <memoria/prototypes/bt/walkers/bt_skip_walkers.hpp>

namespace memoria {
namespace bt {


template <typename Types, typename LeafPath_>
struct RankWalkerTypes: Types {
    using LeafPath = IntList<ListHead<LeafPath_>::Value>;
    using RankLeafPath = LeafPath_;
};




template <typename Types>
class RankForwardWalker: public SkipForwardWalkerBase<Types, RankForwardWalker<Types>> {

    using MyType = RankForwardWalker<Types>;

    using Base      = SkipForwardWalkerBase<Types, RankForwardWalker<Types>>;
    using Iterator  = typename Base::Iterator;

    using RankLeafSubstreamPath = typename Types::RankLeafPath;

    using CtrSizeT = typename Types::CtrSizeT;

    CtrSizeT rank_ = 0;
    int32_t symbol_;

    SeqOpType seq_op_ = SeqOpType::EQ;

    struct BranchFn {
        template <typename StreamType>
        auto stream(const StreamType& stream, int32_t symbol, int32_t start, int32_t end, SeqOpType seq_op)
        {
            return stream.sum_for_rank(start, end, symbol, seq_op);
        }
    };

    struct LeafFn {
        template <typename StreamType>
        auto stream(const StreamType& stream, int32_t symbol, int32_t start, int32_t end, SeqOpType seq_op)
        {
            return stream.rank(start, end, symbol, seq_op);
        }
    };


public:
    using Base::processCmd;

    RankForwardWalker(int32_t, int32_t symbol, CtrSizeT target, SeqOpType seq_op):
        Base(target),
        symbol_(symbol),
        seq_op_(seq_op)
    {}

    RankForwardWalker(int32_t symbol, CtrSizeT target, SeqOpType seq_op):
        Base(target),
        symbol_(symbol),
        seq_op_(seq_op)
    {}


    CtrSizeT rank() const {
        return rank_;
    }

    CtrSizeT result() const {
        return rank_;
    }

    SeqOpType seq_op() const {
        return seq_op_;
    }

    template <typename CtrT, typename NodeT, typename... Args>
    void processCmd(BranchNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end, Args&&... args)
    {
        Base::processCmd(node, cmd, start, end, std::forward<Args>(args)...);

        if (cmd == WalkCmd::FIX_TARGET)
        {
            using BranchPath = typename NodeT::template BuildBranchPath<RankLeafSubstreamPath>;
            const int32_t index = NodeT::template translateLeafIndexToBranchIndex<RankLeafSubstreamPath>(symbol_);

            rank_ -= node->template processStream<BranchPath>(BranchFn(), index, end, end + 1, seq_op());
        }
    }

    CtrSizeT finish(Iterator& iter, int32_t idx, WalkCmd cmd)
    {
        Base::finish(iter, idx, cmd);

        return rank_;
    }


    template <typename Node, typename Result>
    void postProcessBranchNode(Node& node, WalkDirection direction, int32_t start, Result&& result)
    {
        using BranchPath = typename Node::template BuildBranchPath<RankLeafSubstreamPath>;
        const int32_t index = Node::template translateLeafIndexToBranchIndex<RankLeafSubstreamPath>(symbol_);

        rank_ += node.template processStream<BranchPath>(BranchFn(), index, start, result.local_pos(), seq_op()).get_or_throw();
    }

    template <typename Node, typename Result>
    void postProcessLeafNode(Node& node, WalkDirection direction, int32_t start, Result&& result)
    {
        auto val = node.template processStream<RankLeafSubstreamPath>(LeafFn(), symbol_, start, result.local_pos(), seq_op()).get_or_throw();
        rank_ += val;
    }
};







template <
    typename Types
>
class RankBackwardWalker: public SkipBackwardWalkerBase<Types, RankBackwardWalker<Types>> {

    using Base      = SkipBackwardWalkerBase<Types, RankBackwardWalker<Types>>;
    using Iterator  = typename Base::Iterator;

    using RankLeafSubstreamPath = typename Types::RankLeafPath;

    using CtrSizeT = typename Types::CtrSizeT;
    CtrSizeT rank_ = 0;

    int32_t symbol_;

    SeqOpType seq_op_;

    struct BranchFn {
        template <typename StreamType>
        auto stream(const StreamType& stream, int32_t symbol, int32_t start, int32_t end, SeqOpType seq_op)
        {
            if (start > stream.size()) start = stream.size() - 1;
            return stream.sum_for_rank(end + 1, start + 1, symbol, seq_op);
        }
    };

    struct LeafFn {
        template <typename StreamType>
        auto stream(const StreamType& stream, int32_t symbol, int32_t start, int32_t end, SeqOpType seq_op)
        {
            if (start > stream.size()) start = stream.size();
            if (end < 0) end = 0;

            return stream.rank(end, start, symbol, seq_op);
        }
    };



public:
    using Base::processCmd;

    RankBackwardWalker(int32_t, int32_t symbol, CtrSizeT target, SeqOpType seq_op):
        Base(target),
        symbol_(symbol),
        seq_op_(seq_op)
    {}

    RankBackwardWalker(int32_t symbol, CtrSizeT target, SeqOpType seq_op):
        Base(target),
        symbol_(symbol),
        seq_op_(seq_op)
    {}

    CtrSizeT rank() const {
        return rank_;
    }

    CtrSizeT result() const {
        return rank_;
    }

    SeqOpType seq_op() const {
        return seq_op_;
    }

//    template <int32_t StreamIdx, typename StreamType>
//    void postProcessLeafStream(const StreamType* stream, int32_t start, int32_t end)
//    {
//        if (start > stream->size()) start = stream->size();
//        if (end < 0) end = 0;
//
//        rank_ += stream->rank(end, start, symbol_);
//    }
//
//    template <int32_t StreamIdx, typename StreamType>
//    void postProcessBranchStream(const StreamType* stream, int32_t start, int32_t end)
//    {
//        if (start > stream->size()) start = stream->size() - 1;
//
//        rank_ += stream->sum(Base::branchIndex(symbol_), end + 1, start + 1);
//    }


    template <typename Node, typename Result>
    void postProcessBranchNode(const Node& node, WalkDirection direction, int32_t start, Result&& result)
    {
        using BranchPath = typename Node::template BuildBranchPath<RankLeafSubstreamPath>;
        const int32_t index = Node::template translateLeafIndexToBranchIndex<RankLeafSubstreamPath>(symbol_);

        rank_ += node.template processStream<BranchPath>(BranchFn(), index, start, result.local_pos(), seq_op()).get_or_throw();
    }

    template <typename Node, typename Result>
    void postProcessLeafNode(const Node& node, WalkDirection direction, int32_t start, Result&& result)
    {
        rank_ += node.template processStream<RankLeafSubstreamPath>(LeafFn(), symbol_, start, result.local_pos(), seq_op()).get_or_throw();
    }


    template <typename CtrT, typename NodeT, typename... Args>
    void processCmd(const BranchNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end, Args&&... args)
    {
        using Node = BranchNodeSO<CtrT, NodeT>;

        Base::processCmd(node, cmd, start, end, std::forward<Args>(args)...);

        if (cmd == WalkCmd::FIX_TARGET)
        {
            using BranchPath = typename Node::template BuildBranchPath<RankLeafSubstreamPath>;
            const int32_t index = Node::template translateLeafIndexToBranchIndex<RankLeafSubstreamPath>(symbol_);

            rank_ -= node.template processStream<BranchPath>(BranchFn(), index, end + 1, end + 2, seq_op()).get_or_throw();
        }
    }



//    template <int32_t StreamIdx, typename Tree>
//    void process_branch_cmd(const Tree* tree, WalkCmd cmd, int32_t index, int32_t start, int32_t end)
//    {
//        Base::template process_branch_cmd<StreamIdx, Tree>(tree, cmd, index, start, end);
//
//        if (cmd == WalkCmd::FIX_TARGET)
//        {
//            rank_ -= tree->value(Base::branchIndex(symbol_), end + 1);
//        }
//    }

    CtrSizeT finish(Iterator& iter, int32_t idx, WalkCmd cmd)
    {
        Base::finish(iter, idx, cmd);

        return rank_;
    }
};





}}
