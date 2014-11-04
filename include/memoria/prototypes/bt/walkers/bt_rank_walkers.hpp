
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_RANK_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_RANK_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_skip_walkers.hpp>

namespace memoria {
namespace bt1     {


template <
    typename Types,
    Int Stream,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class RankForwardWalker: public SkipForwardWalkerBase<
                                    Types,
                                    Stream,
                                    IteratorPrefixFn,
                                    RankForwardWalker<Types, Stream, IteratorPrefixFn>> {

    using Base      = SkipForwardWalkerBase<Types, Stream, IteratorPrefixFn, RankForwardWalker<Types, Stream, IteratorPrefixFn>>;
    using Key       = typename Base::Key;
    using Iterator  = typename Base::Iterator;

    using CtrSizeT = typename Types::CtrSizeT;
    CtrSizeT rank_ = 0;

    Int branch_rank_index_;
    Int symbol_;

public:

    RankForwardWalker(Int stream, Int branch_rank_index, Int symbol, Key target):
        Base(stream, 0, 0, target),
        branch_rank_index_(branch_rank_index),
        symbol_(symbol)
    {}

    template <Int StreamIdx, typename StreamType>
    void postProcessLeafStream(const StreamType* stream, Int start, Int end)
    {
        return stream->rank(start, end, symbol_);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessNonLeafStream(const StreamType* stream, Int start, Int end, Int symbol)
    {
        return stream->sum(branch_rank_index_, start, end);
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        Base::finish(iter, idx);

        return rank_;
    }
};



template <
    typename Types,
    Int Stream,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class RankBackwardWalker: public SkipBackwardWalkerBase<
                                    Types,
                                    Stream,
                                    IteratorPrefixFn,
                                    RankBackwardWalker<Types, Stream, IteratorPrefixFn>> {

    using Base      = SkipBackwardWalkerBase<Types, Stream, IteratorPrefixFn, RankBackwardWalker<Types, Stream, IteratorPrefixFn>>;
    using Key       = typename Base::Key;
    using Iterator  = typename Base::Iterator;

    using CtrSizeT = typename Types::CtrSizeT;
    CtrSizeT rank_ = 0;

    Int branch_rank_index_;
    Int symbol_;

public:

    RankBackwardWalker(Int stream, Int branch_rank_index, Int symbol, Key target):
        Base(stream, 0, 0, target),
        branch_rank_index_(branch_rank_index),
        symbol_(symbol)
    {}

    template <Int StreamIdx, typename StreamType>
    void postProcessLeafStream(const StreamType* stream, Int start, Int end)
    {
        return stream->rank(start, end, symbol_);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessNonLeafStream(const StreamType* stream, Int start, Int end, Int symbol)
    {
        return stream->sum(branch_rank_index_, start, end);
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        Base::finish(iter, idx);

        return rank_;
    }
};





}
}

#endif
