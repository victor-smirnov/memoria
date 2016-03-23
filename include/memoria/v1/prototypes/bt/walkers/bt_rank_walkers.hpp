
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


/*****************************************************************************************/

template <
    typename Types
>
class RankForwardWalker: public SkipForwardWalkerBase<Types, RankForwardWalker<Types>> {

    using Base      = SkipForwardWalkerBase<Types, RankForwardWalker<Types>>;
    using Iterator  = typename Base::Iterator;

    using CtrSizeT = typename Types::CtrSizeT;
    CtrSizeT rank_ = 0;

    Int symbol_;

public:

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

    template <Int StreamIdx, typename StreamType>
    void postProcessLeafStream(const StreamType* stream, Int start, Int end)
    {
        rank_ += stream->rank(start, end, symbol_);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessBranchStream(const StreamType* stream, Int start, Int end)
    {
        rank_ += stream->sum(Base::branchIndex(symbol_), start, end);
    }

    template <Int StreamIdx, typename Tree>
    void process_branch_cmd(const Tree* tree, WalkCmd cmd, Int index, Int start, Int end)
    {
        Base::template process_branch_cmd<StreamIdx, Tree>(tree, cmd, index, start, end);

        if (cmd == WalkCmd::FIX_TARGET)
        {
            rank_ -= tree->value(Base::branchIndex(symbol_), end);
        }
    }

    CtrSizeT finish(Iterator& iter, Int idx, WalkCmd cmd)
    {
        Base::finish(iter, idx, cmd);

        return rank_;
    }
};


template <
    typename Types
>
class RankBackwardWalker: public SkipBackwardWalkerBase<Types, RankBackwardWalker<Types>> {

    using Base      = SkipBackwardWalkerBase<Types, RankBackwardWalker<Types>>;
    using Iterator  = typename Base::Iterator;

    using CtrSizeT = typename Types::CtrSizeT;
    CtrSizeT rank_ = 0;

    Int symbol_;

public:

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


    template <Int StreamIdx, typename StreamType>
    void postProcessLeafStream(const StreamType* stream, Int start, Int end)
    {
        if (start > stream->size()) start = stream->size();
        if (end < 0) end = 0;

        rank_ += stream->rank(end, start, symbol_);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessBranchStream(const StreamType* stream, Int start, Int end)
    {
        if (start > stream->size()) start = stream->size() - 1;

        rank_ += stream->sum(Base::branchIndex(symbol_), end + 1, start + 1);
    }

    template <Int StreamIdx, typename Tree>
    void process_branch_cmd(const Tree* tree, WalkCmd cmd, Int index, Int start, Int end)
    {
        Base::template process_branch_cmd<StreamIdx, Tree>(tree, cmd, index, start, end);

        if (cmd == WalkCmd::FIX_TARGET)
        {
            rank_ -= tree->value(Base::branchIndex(symbol_), end + 1);
        }
    }

    CtrSizeT finish(Iterator& iter, Int idx, WalkCmd cmd)
    {
        Base::finish(iter, idx, cmd);

        return rank_;
    }
};





}
}}