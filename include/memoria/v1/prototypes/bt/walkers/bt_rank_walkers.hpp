
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

#include <memoria/v1/prototypes/bt/walkers/bt_skip_walkers.hpp>

namespace memoria {
namespace v1 {
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
    Int symbol_;

    struct BranchFn {
        template <typename StreamType>
        auto stream(const StreamType* stream, Int symbol, Int start, Int end)
        {
            return stream->sum(symbol, start, end);
        }
    };

    struct LeafFn {
        template <typename StreamType>
        auto stream(const StreamType* stream, Int symbol, Int start, Int end)
        {
            return stream->rank(start, end, symbol);
        }
    };


public:
    using Base::processCmd;

    RankForwardWalker(Int, Int symbol, CtrSizeT target):
        Base(target),
        symbol_(symbol)
    {}

    RankForwardWalker(Int symbol, CtrSizeT target):
        Base(target),
        symbol_(symbol)
    {}


    CtrSizeT rank() const {
        return rank_;
    }

    CtrSizeT result() const {
        return rank_;
    }

    template <typename NodeTypes, typename... Args>
    void processCmd(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end, Args&&... args)
    {
        using Node = bt::BranchNode<NodeTypes>;

        Base::processCmd(node, cmd, start, end, std::forward<Args>(args)...);

        if (cmd == WalkCmd::FIX_TARGET)
        {
            using BranchPath = typename Node::template BuildBranchPath<RankLeafSubstreamPath>;
            const Int index = Node::template translateLeafIndexToBranchIndex<RankLeafSubstreamPath>(symbol_);

            rank_ -= node->template processStream<BranchPath>(BranchFn(), index, end, end + 1);
        }
    }

    CtrSizeT finish(Iterator& iter, Int idx, WalkCmd cmd)
    {
        Base::finish(iter, idx, cmd);

        return rank_;
    }


    template <typename Node, typename Result>
    void postProcessBranchNode(const Node* node, WalkDirection direction, Int start, Result&& result)
    {
        using BranchPath = typename Node::template BuildBranchPath<RankLeafSubstreamPath>;
        const Int index = Node::template translateLeafIndexToBranchIndex<RankLeafSubstreamPath>(symbol_);

        rank_ += node->template processStream<BranchPath>(BranchFn(), index, start, result.idx());
    }

    template <typename Node, typename Result>
    void postProcessLeafNode(const Node* node, WalkDirection direction, Int start, Result&& result)
    {
        rank_ += node->template processStream<RankLeafSubstreamPath>(LeafFn(), symbol_, start, result.idx());
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

    Int symbol_;


    struct BranchFn {
        template <typename StreamType>
        auto stream(const StreamType* stream, Int symbol, Int start, Int end)
        {
            if (start > stream->size()) start = stream->size() - 1;

            return stream->sum(symbol, end + 1, start + 1);
        }
    };

    struct LeafFn {
        template <typename StreamType>
        auto stream(const StreamType* stream, Int symbol, Int start, Int end)
        {
            if (start > stream->size()) start = stream->size();
            if (end < 0) end = 0;

            return stream->rank(end, start, symbol);
        }
    };



public:
    using Base::processCmd;

    RankBackwardWalker(Int, Int symbol, CtrSizeT target):
        Base(target),
        symbol_(symbol)
    {}

    RankBackwardWalker(Int symbol, CtrSizeT target):
        Base(target),
        symbol_(symbol)
    {}

    CtrSizeT rank() const {
        return rank_;
    }

    CtrSizeT result() const {
        return rank_;
    }


//    template <Int StreamIdx, typename StreamType>
//    void postProcessLeafStream(const StreamType* stream, Int start, Int end)
//    {
//        if (start > stream->size()) start = stream->size();
//        if (end < 0) end = 0;
//
//        rank_ += stream->rank(end, start, symbol_);
//    }
//
//    template <Int StreamIdx, typename StreamType>
//    void postProcessBranchStream(const StreamType* stream, Int start, Int end)
//    {
//        if (start > stream->size()) start = stream->size() - 1;
//
//        rank_ += stream->sum(Base::branchIndex(symbol_), end + 1, start + 1);
//    }


    template <typename Node, typename Result>
    void postProcessBranchNode(const Node* node, WalkDirection direction, Int start, Result&& result)
    {
        using BranchPath = typename Node::template BuildBranchPath<RankLeafSubstreamPath>;
        const Int index = Node::template translateLeafIndexToBranchIndex<RankLeafSubstreamPath>(symbol_);

        rank_ += node->template processStream<BranchPath>(BranchFn(), index, start, result.idx());
    }

    template <typename Node, typename Result>
    void postProcessLeafNode(const Node* node, WalkDirection direction, Int start, Result&& result)
    {
        rank_ += node->template processStream<RankLeafSubstreamPath>(LeafFn(), symbol_, start, result.idx());
    }


    template <typename NodeTypes, typename... Args>
    void processCmd(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end, Args&&... args)
    {
        using Node = bt::BranchNode<NodeTypes>;

        Base::processCmd(node, cmd, start, end, std::forward<Args>(args)...);

        if (cmd == WalkCmd::FIX_TARGET)
        {
            using BranchPath = typename Node::template BuildBranchPath<RankLeafSubstreamPath>;
            const Int index = Node::template translateLeafIndexToBranchIndex<RankLeafSubstreamPath>(symbol_);

            rank_ -= node->template processStream<BranchPath>(BranchFn(), index, end + 1, end + 2);
        }
    }



//    template <Int StreamIdx, typename Tree>
//    void process_branch_cmd(const Tree* tree, WalkCmd cmd, Int index, Int start, Int end)
//    {
//        Base::template process_branch_cmd<StreamIdx, Tree>(tree, cmd, index, start, end);
//
//        if (cmd == WalkCmd::FIX_TARGET)
//        {
//            rank_ -= tree->value(Base::branchIndex(symbol_), end + 1);
//        }
//    }

    CtrSizeT finish(Iterator& iter, Int idx, WalkCmd cmd)
    {
        Base::finish(iter, idx, cmd);

        return rank_;
    }
};





}
}}
