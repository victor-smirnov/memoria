
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/bt_walkers.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>

namespace memoria {
namespace louds     {

template <typename Types>
class SkipForwardWalker: public bt::FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {
    typedef bt::FindForwardWalkerBase<Types, SkipForwardWalker<Types>>          Base;
    typedef typename Base::Key                                                  Key;

    BigInt rank1_ = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;


    SkipForwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
    {}

    template <Int Idx, typename Tree>
    ResultType stream(Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        Int size = stream->size();

        if (result.idx() < size)
        {
            rank1_ += stream->sum(2, start, result.idx());
        }
        else {
            rank1_ += stream->sum(2, start, size);
        }
    }

    template <Int Idx, typename StreamTypes>
    ResultType stream(const PkdFSSeq<StreamTypes>* seq, Int start)
    {
        auto& sum = Base::sum_;

        BigInt offset = Base::target_ - sum;

        Int size = seq != nullptr? seq->size() : 0;

        if (start + offset < size)
        {
            sum += offset;

            rank1_ += seq->rank(start >= 0 ? start : 0, start + offset, 1);

            return start + offset;
        }
        else {
            sum += (size - start);

            rank1_ += seq->rank(start >= 0 ? start : 0, seq->size(), 1);

            return size;
        }
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        iter.cache().add(this->sum_, rank1_);

        return this->sum_;
    }
};

template <typename Types>
class SkipBackwardWalker: public bt::FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {
    typedef bt::FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>>        Base;
    typedef typename Base::Key                                                  Key;

    BigInt rank1_ = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;

    SkipBackwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
    {
        Base::search_type_ = SearchType::GT;
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start) {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        if (result.idx() >= 0)
        {
            rank1_ += stream->sum(2, result.idx() + 1, start + 1);
        }
        else {
            rank1_ += stream->sum(2, 0, start + 1);
        }
    }


    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdFSSeq<TreeTypes>* seq, Int start)
    {
        BigInt offset = Base::target_ - Base::sum_;

        auto& sum = Base::sum_;

        if (start - offset >= 0)
        {
            sum += offset;

            rank1_ += seq->rank(start - offset, start, 1);

            return start - offset;
        }
        else {
            sum += start;

            rank1_ += seq->rank(0, start, 1);

            return -1;
        }
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        if (idx >= 0)
        {
            iter.cache().sub(this->sum_, rank1_);
        }
        else {
            iter.cache().setup(-1, 0);
        }

        return this->sum_;
    }
};





}
}
