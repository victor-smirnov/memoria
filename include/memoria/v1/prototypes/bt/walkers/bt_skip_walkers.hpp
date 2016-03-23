
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/walkers/bt_find_walkers.hpp>

namespace memoria {
namespace bt {

/***************************************************************************************/


template <
    typename Types,
    typename MyType
>
class SkipForwardWalkerBase: public FindForwardWalkerBase<Types, MyType> {
protected:
    using Base  = FindForwardWalkerBase<Types, MyType>;
    using CtrSizeT = typename Types::CtrSizeT;

public:

    SkipForwardWalkerBase(CtrSizeT target):
        Base(0, target, SearchType::GT)
    {}


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
        auto& sum = Base::sum_;

        BigInt offset = Base::target_ - sum;

        if (tree != nullptr)
        {
            Int size = tree->size();

            if (start + offset < size)
            {
                sum += offset;

                return StreamOpResult(start + offset, start, false);
            }
            else {
                sum += (size - start);

                return StreamOpResult(size, start, true);
            }
        }
        else {
            return StreamOpResult(0, start, true);
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};



template <
    typename Types
>
class SkipForwardWalker: public SkipForwardWalkerBase<Types, SkipForwardWalker<Types>> {
    using Base  = SkipForwardWalkerBase<Types, SkipForwardWalker<Types>>;

    using CtrSizeT   = typename Base::CtrSizeT;

public:

    SkipForwardWalker(CtrSizeT target):
        Base(target)
    {}
};




template <
    typename Types,
    typename MyType
>
class SkipBackwardWalkerBase: public FindBackwardWalkerBase<Types,MyType> {
protected:
    using Base  = FindBackwardWalkerBase<Types,MyType>;

    using CtrSizeT = typename Types::CtrSizeT;

public:

    SkipBackwardWalkerBase(CtrSizeT target):
        Base(0, target, SearchType::GE)
    {}


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
        auto size = tree != nullptr ? tree->size() : 0;

        if (start > size) start = size;

        if (start >= 0)
        {
            BigInt offset = Base::target_ - Base::sum_;

            auto& sum = Base::sum_;

            if (tree != nullptr)
            {
                if (start - offset >= 0)
                {
                    sum += offset;

                    return StreamOpResult(start - offset, start, false);
                }
                else {
                    sum += start;

                    return StreamOpResult(-1, start, true);
                }
            }
            else {
                return StreamOpResult(-1, start, true, true);
            }
        }
        else {
            return StreamOpResult(-1, start, true, true);
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};


template <
    typename Types
>
class SkipBackwardWalker: public SkipBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {
    using Base  = SkipBackwardWalkerBase<Types,SkipBackwardWalker<Types>>;

    using CtrSizeT = typename Base::CtrSizeT;

public:
    SkipBackwardWalker(CtrSizeT target):
        Base(target)
    {}
};




}
}
