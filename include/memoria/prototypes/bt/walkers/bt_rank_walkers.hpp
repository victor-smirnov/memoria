
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_RANK_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_RANK_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_skip_walkers.hpp>

namespace memoria {
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

    CtrSizeT finish(Iterator& iter, Int idx, WalkCmd cmd)
    {
        Base::finish(iter, idx, cmd);

        return rank_;
    }
};





}
}

#endif
