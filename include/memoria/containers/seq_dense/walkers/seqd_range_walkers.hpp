
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_CONTAINERS_SEQDENSE_RANGE_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_RANGE_WALKERS_HPP

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

namespace memoria   {
namespace seq_dense {

template <typename Types>
class SkipForwardWalker: public bt::FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {

    typedef bt::FindForwardWalkerBase<Types, SkipForwardWalker<Types>>          Base;
    typedef typename Base::Key                                                  Key;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;


    SkipForwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
    {}

    template <Int Idx, typename Tree>
    ResultType stream(Tree* tree, Int start) {
        return Base::template stream<Idx>(tree, start);
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

            return start + offset;
        }
        else {
            sum += (size - start);

            return size;
        }
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        iter.cache().add(this->sum_);

        return this->sum_;
    }
};

template <typename Types>
class SkipBackwardWalker: public bt::FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {

    typedef bt::FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>>        Base;
    typedef typename Base::Key                                                  Key;

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


    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdFSSeq<TreeTypes>* seq, Int start)
    {
        BigInt offset = Base::target_ - Base::sum_;

        auto& sum = Base::sum_;

        if (start - offset >= 0)
        {
            sum += offset;
            return start - offset;
        }
        else {
            sum += start;
            return -1;
        }
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        iter.cache().sub(this->sum_);

        return this->sum_;
    }
};





}
}

#endif
