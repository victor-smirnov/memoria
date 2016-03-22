
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/typelist.hpp>

namespace memoria    {

namespace {

    template <typename List, template <typename, typename> class FoldingFn, typename State = EmptyType> struct FoldTLRightT;

    template <typename Head, typename... Tail, template <typename, typename> class FoldingFn, typename State>
    struct FoldTLRightT<TL<Head, Tail...>, FoldingFn, State>
    {
        using NewState  = FoldingFn<State, Head>;
        using Type      = typename FoldTLRightT<TL<Tail...>, FoldingFn, NewState>::Type;
    };

    template <typename Element, template <typename, typename> class FoldingFn, typename State>
    struct FoldTLRightT<TL<Element>, FoldingFn, State>
    {
        using Type = FoldingFn<State, Element>;
    };

    template <template <typename, typename> class FoldingFn, typename State>
    struct FoldTLRightT<TL<>, FoldingFn, State>
    {
        using Type = FoldingFn<State, EmptyType>;
    };



    template <typename List, template <typename, typename> class FoldingFn> struct FoldTLLeftT;

    template <typename Head, typename... Tail, template <typename, typename> class FoldingFn>
    struct FoldTLLeftT<TL<Head, Tail...>, FoldingFn>
    {
        using Type = FoldingFn<typename FoldTLLeftT<TL<Tail...>, FoldingFn>::Type, Head>;
    };

    template <typename Head, template <typename, typename> class FoldingFn>
    struct FoldTLLeftT<TL<Head>, FoldingFn>
    {
        using Type = FoldingFn<EmptyType, Head>;
    };

    template <template <typename, typename> class FoldingFn>
    struct FoldTLLeftT<TL<>, FoldingFn>
    {
        using Type = FoldingFn<EmptyType, EmptyType>;
    };

}

template <typename List, template <typename, typename> class FoldingFn, typename State = EmptyType>
using FoldTLRight = typename FoldTLRightT<List, FoldingFn, State>::Type;

template <typename List, template <typename, typename> class FoldingFn>
using FoldTLLeft = typename FoldTLLeftT<List, FoldingFn>::Type;

}
