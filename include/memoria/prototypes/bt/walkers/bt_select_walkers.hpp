
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SELECT_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SELECT_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>

namespace memoria {
namespace bt1     {


template <
    typename Types,
    typename IteratorPrefixFn,
    typename MyType
>
class SelectForwardWalkerBase: public FindForwardWalkerBase<Types, IteratorPrefixFn, MyType> {
protected:
    using Base  = FindForwardWalkerBase<Types, IteratorPrefixFn, MyType>;
    using Key   = typename Base::Key;

public:

    SelectForwardWalkerBase(Int stream, Int branch_index, Int symbol, Key target):
        Base(stream, branch_index, symbol, target, SearchType::GE)
    {}


    template <Int StreamIdx, typename Seq>
    Int find_leaf(const Seq* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq);

        auto& sum       = Base::sum_;
        auto symbol     = Base::leaf_index();

        BigInt rank     = Base::target_ - sum;
        auto result     = self().template select<StreamIdx>(seq, start, symbol, rank);

        this->end_      = !result.is_found();

        IteratorPrefixFn fn;

        if (result.is_found())
        {
            fn.processLeafFw(seq, std::get<StreamIdx>(Base::prefix_), start, result.idx());

            return result.idx();
        }
        else {
            Int size = seq->size();

            sum  += result.rank();

            fn.processLeafFw(seq, std::get<StreamIdx>(Base::prefix_), start, size);

            return size;
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};




template <
    typename Types,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class SelectForwardWalker: public SelectForwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    SelectForwardWalker<Types, IteratorPrefixFn>> {

    using Base  = SelectForwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    SelectForwardWalker<Types, IteratorPrefixFn>
    >;
    using Key   = typename Base::Key;

public:
    SelectForwardWalker(Int stream, Int branch_index, Int symbol, Key target):
        Base(stream, branch_index, symbol, target)
    {}

    template <Int StreamIdx, typename Seq>
    SelectResult select(const Seq* seq, Int start, Int symbol, BigInt rank)
    {
        return seq->selectFw(start, symbol, rank);
    }
};




template <
    typename Types,
    typename IteratorPrefixFn,
    typename MyType
>
class SelectBackwardWalkerBase: public FindBackwardWalkerBase<Types, IteratorPrefixFn, MyType> {
protected:
    using Base  = FindBackwardWalkerBase<Types, IteratorPrefixFn, MyType>;
    using Key   = typename Base::Key;

public:

    SelectBackwardWalkerBase(Int stream, Int branch_index, Int symbol, Key target):
        Base(stream, branch_index, symbol, target, SearchType::GE)
    {}


    template <Int StreamIdx, typename Seq>
    Int find_leaf(const Seq* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq);

        BigInt target   = Base::target_ - Base::sum_;

        auto& sum       = Base::sum_;
        auto symbol     = Base::leaf_index();
        auto result     = self().template select<StreamIdx>(seq, start, symbol, target);

        this->end_      = !result.is_found();

        IteratorPrefixFn fn;

        if (result.is_found())
        {
            fn.processLeafBw(seq, std::get<StreamIdx>(Base::prefix_), result.idx(), start);

            return result.idx();
        }
        else {
            sum += result.rank();

            fn.processLeafBw(seq, std::get<StreamIdx>(Base::prefix_), 0, start);

            return -1;
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};




template <
    typename Types,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class SelectBackwardWalker: public SelectBackwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    SelectBackwardWalker<Types, IteratorPrefixFn>> {

    using Base  = SelectBackwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    SelectBackwardWalker<
                        Types,
                        IteratorPrefixFn
                    >
    >;
    using Key   = typename Base::Key;

public:

    SelectBackwardWalker(Int stream, Int block, Key target):
        Base(stream, block, block, target)
    {}

    template <Int StreamIdx, typename Seq>
    SelectResult select(const Seq* seq, Int start, Int symbol, BigInt rank)
    {
        return seq->selectBw(start, symbol, rank);
    }
};







/**********************************************************************/

template <
    typename Types,
    typename MyType
>
class SelectForwardWalkerBase2: public FindForwardWalkerBase2<Types, MyType> {
protected:
    using Base  = FindForwardWalkerBase2<Types, MyType>;
    using CtrSizeT   = typename Types::CtrSizeT;

public:

    SelectForwardWalkerBase2(Int symbol, CtrSizeT rank):
        Base(symbol, rank, SearchType::GE)
    {}


    template <Int StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq);

        auto& sum       = Base::sum_;
        auto symbol     = Base::leaf_index();

        BigInt rank     = Base::target_ - sum;
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
class SelectForwardWalker2: public SelectForwardWalkerBase2<Types,SelectForwardWalker2<Types>> {

    using Base  = SelectForwardWalkerBase2<Types,SelectForwardWalker2<Types>>;
    using CtrSizeT   = typename Base::CtrSizeT;

public:
    SelectForwardWalker2(Int symbol, CtrSizeT rank):
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
class SelectBackwardWalkerBase2: public FindBackwardWalkerBase2<Types, MyType> {
protected:
    using Base  = FindBackwardWalkerBase2<Types, MyType>;
    using CtrSizeT   = typename Types::CtrSizeT;

public:

    SelectBackwardWalkerBase2(Int symbol, CtrSizeT rank):
        Base(symbol, rank, SearchType::GE)
    {}


    template <Int StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq);

        auto size = seq->size();

        if (start > size) start = size;

        BigInt target   = Base::target_ - Base::sum_;

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
class SelectBackwardWalker2: public SelectBackwardWalkerBase2<Types, SelectBackwardWalker2<Types>> {

    using Base  = SelectBackwardWalkerBase2<Types, SelectBackwardWalker2<Types>>;
    using CtrSizeT   = typename Base::CtrSizeT;

public:

    SelectBackwardWalker2(Int symbol, CtrSizeT target):
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

#endif
