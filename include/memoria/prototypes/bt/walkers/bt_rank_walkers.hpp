
// Copyright Victor Smirnov 2013-2015.
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
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class RankForwardWalker: public SkipForwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    RankForwardWalker<
                                        Types,
                                        IteratorPrefixFn
                                    >
> {

    using Base      = SkipForwardWalkerBase<
                        Types,
                        IteratorPrefixFn,
                        RankForwardWalker<
                            Types,
                            IteratorPrefixFn
                        >
    >;
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
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class RankBackwardWalker: public SkipBackwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    RankBackwardWalker<
                                        Types,
                                        IteratorPrefixFn
                                    >
> {

    using Base      = SkipBackwardWalkerBase<
                        Types,
                        IteratorPrefixFn,
                        RankBackwardWalker<
                            Types,
                            IteratorPrefixFn
                        >
    >;
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




/*****************************************************************************************/

template <
    typename Types
>
class RankForwardWalker2: public SkipForwardWalkerBase2<Types, RankForwardWalker2<Types>> {

    using Base      = SkipForwardWalkerBase2<Types, RankForwardWalker2<Types>>;
    using Key       = typename Base::Key;
    using Iterator  = typename Base::Iterator;

    using CtrSizeT = typename Types::CtrSizeT;
    CtrSizeT rank_ = 0;

    Int symbol_;

public:

    RankForwardWalker2(Int, Int symbol, Key target):
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

    CtrSizeT finish(Iterator& iter, Int idx)
    {
        Base::finish(iter, idx);

        return rank_;
    }
};


template <
    typename Types
>
class RankBackwardWalker2: public SkipBackwardWalkerBase2<Types, RankBackwardWalker2<Types>> {

    using Base      = SkipBackwardWalkerBase2<Types, RankBackwardWalker2<Types>>;
    using Key       = typename Base::Key;
    using Iterator  = typename Base::Iterator;

    using CtrSizeT = typename Types::CtrSizeT;
    CtrSizeT rank_ = 0;

    Int symbol_;

public:

    RankBackwardWalker2(Int, Int symbol, Key target):
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

    CtrSizeT finish(Iterator& iter, Int idx)
    {
        Base::finish(iter, idx);

        return rank_;
    }
};





}
}

#endif
