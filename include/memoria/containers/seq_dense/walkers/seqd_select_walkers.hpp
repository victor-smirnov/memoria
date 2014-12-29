
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_CONTAINERS_SEQDENSE_SELECT_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_SELECT_WALKERS_HPP

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

namespace memoria   {
namespace seq_dense {




template <typename Types>
class SelectForwardWalker: public bt::FindForwardWalkerBase<Types, SelectForwardWalker<Types>> {

    using Base = bt::FindForwardWalkerBase<Types, SelectForwardWalker<Types>>;
    typedef typename Base::Key                                                              Key;

    BigInt pos_ = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;


    SelectForwardWalker(Int stream, Int index, Key target): Base(stream, index + 1, target)
    {
        Base::search_type_ = SearchType::GE;
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        Int size    = stream->size();

        if (result.idx() < size)
        {
            pos_ += stream->sum(0, start, result.idx());
        }
        else {
            pos_ += stream->sum(0, start, size);
        }
    }

    template <Int Idx, typename StreamTypes>
    ResultType stream(const PkdFSSeq<StreamTypes>* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq != nullptr);

        auto& sum       = Base::sum_;
        auto symbol     = Base::index_ - 1;

        BigInt target   = Base::target_ - sum;
        auto result     = seq->selectFw(start, symbol, target);

        if (result.is_found())
        {
            pos_ += result.idx() - start;



            return result.idx();
        }
        else {
            Int size = seq->size();

            sum  += result.rank();
            pos_ += (size - start);

            return size;
        }
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        iter.cache().add(pos_);

        return pos_;
    }
};


template <typename Types>
class SelectForwardWalker2: public bt::SelectForwardWalkerBase<Types> {

    using Base = bt::SelectForwardWalkerBase<Types>;
    typedef typename Base::Key                                                  Key;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;


    SelectForwardWalker2(Int stream, Int index, Key target): Base(stream, index + 1, index, target, 0)
    {}

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }


    template <Int Idx, typename StreamTypes>
    ResultType stream(const PkdFSSeq<StreamTypes>* seq, Int start)
    {
        return Base::template select<Idx>(seq, start);
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        iter.cache().add(this->pos_);

        return this->pos_;
    }
};




template <typename Types>
class SelectBackwardWalker: public bt::FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>> {

    using Base = bt::FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>>;
    typedef typename Base::Key                                                                  Key;


    BigInt pos_ = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;

    SelectBackwardWalker(Int stream, Int index, Key target): Base(stream, index + 1, target)
    {
        Base::search_type_ = SearchType::GT;
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        if (result.idx() >= 0)
        {
            pos_ += stream->sum(0, result.idx() + 1, start + 1);
        }
        else {
            pos_ += stream->sum(0, 0, start + 1);
        }
    }


    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdFSSeq<TreeTypes>* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq != nullptr);

        BigInt target   = Base::target_ - Base::sum_;

        auto& sum       = Base::sum_;
        auto symbol     = Base::index_ - 1;

        auto result     = seq->selectBw(start, symbol, target);

        if (result.is_found())
        {
            pos_ += start - result.idx();
            return result.idx();
        }
        else {
            pos_ += start;
            sum += result.rank();
            return -1;
        }
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        iter.cache().sub(pos_);

        return pos_;
    }
};

}
}

#endif
