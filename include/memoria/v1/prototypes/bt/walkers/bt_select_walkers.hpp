
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/walkers/bt_walker_base.hpp>

namespace memoria {
namespace bt {


/**********************************************************************/

template <
    typename Types,
    typename MyType
>
class SelectForwardWalkerBase: public FindForwardWalkerBase<Types, MyType> {
protected:
    using Base  = FindForwardWalkerBase<Types, MyType>;
    using CtrSizeT   = typename Types::CtrSizeT;

public:

    SelectForwardWalkerBase(Int symbol, CtrSizeT rank):
        Base(symbol, rank, SearchType::GE)
    {}


    template <Int StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq);

        auto& sum       = Base::sum_;
        auto symbol     = Base::leaf_index();

        CtrSizeT rank   = Base::target_ - sum;
        auto result     = self().template select<StreamIdx>(seq, start, symbol, rank);

        if (result.is_found())
        {
            sum  += rank;
            return StreamOpResult(result.idx(), start, false);
        }
        else {
            Int size = seq->size();

            sum  += result.rank();
            return StreamOpResult(size, start, true);
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};




template <
    typename Types
>
class SelectForwardWalker: public SelectForwardWalkerBase<Types,SelectForwardWalker<Types>> {

    using Base  = SelectForwardWalkerBase<Types,SelectForwardWalker<Types>>;
    using CtrSizeT   = typename Base::CtrSizeT;

public:
    SelectForwardWalker(Int symbol, CtrSizeT rank):
        Base(symbol, rank)
    {}

    template <Int StreamIdx, typename Seq>
    SelectResult select(const Seq* seq, Int start, Int symbol, CtrSizeT rank)
    {
        return seq->selectFw(start, symbol, rank);
    }
};





template <
    typename Types,
    typename MyType
>
class SelectBackwardWalkerBase: public FindBackwardWalkerBase<Types, MyType> {
protected:
    using Base  = FindBackwardWalkerBase<Types, MyType>;
    using CtrSizeT   = typename Types::CtrSizeT;

public:

    SelectBackwardWalkerBase(Int symbol, CtrSizeT rank):
        Base(symbol, rank, SearchType::GE)
    {}


    template <Int StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq);

        auto size = seq->size();

        if (start > size) start = size;

        CtrSizeT target = Base::target_ - Base::sum_;

        auto& sum       = Base::sum_;
        auto symbol     = Base::leaf_index();
        auto result     = self().template select<StreamIdx>(seq, start, symbol, target);

        if (result.is_found())
        {
            sum += target;
            return StreamOpResult(result.idx(), start, false);
        }
        else {
            sum += result.rank();
            return StreamOpResult(-1, start, true);
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};




template <
    typename Types
>
class SelectBackwardWalker: public SelectBackwardWalkerBase<Types, SelectBackwardWalker<Types>> {

    using Base  = SelectBackwardWalkerBase<Types, SelectBackwardWalker<Types>>;
    using CtrSizeT   = typename Base::CtrSizeT;

public:

    SelectBackwardWalker(Int symbol, CtrSizeT target):
        Base(symbol, target)
    {}

    template <Int StreamIdx, typename Seq>
    SelectResult select(const Seq* seq, Int start, Int symbol, CtrSizeT rank)
    {
        return seq->selectBw(start, symbol, rank);
    }
};




}
}
