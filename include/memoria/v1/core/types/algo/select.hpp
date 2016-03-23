
// Copyright Victor Smirnov 2011-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/list/typelist.hpp>
#include <memoria/v1/core/types/list/index.hpp>
#include <memoria/v1/core/types/list/append.hpp>
#include <memoria/v1/core/types/types.hpp>



namespace memoria    {

namespace {

    template <Int Num, typename List, Int Idx> struct SelectT;

    template <Int Num, typename Head, typename ... Tail>
    struct SelectT<Num, TypeList<Head, Tail...>, Num>: HasType<Head> {};

    template <Int Num, typename Head, typename ... Tail, Int Idx>
    struct SelectT<Num, TypeList<Head, Tail...>, Idx>: HasType<
        typename SelectT<Num, TypeList<Tail...>, Idx + 1>::Type
    > {};

    template <Int Num, Int Idx>
    struct SelectT<Num, TypeList<>, Idx>;
}

template <Int Num, typename List>
using Select = typename SelectT<Num, List, 0>::Type;


template <Int Value, typename List, Int idx = 0> struct SelectV;

template <Int Pos, typename T, T Head, T ... Tail>
struct SelectV<Pos, ValueList<T, Head, Tail...>, Pos> {
    static const T Value = Head;
};

template <Int Pos, typename T, T Head, T ... Tail, Int Idx>
struct SelectV<Pos, ValueList<T, Head, Tail...>, Idx> {
    static const T Value = SelectV<Pos, ValueList<T, Tail...>, Idx + 1>::Value;
};

template <Int Value, typename T, Int Idx>
struct SelectV<Value, ValueList<T>, Idx>;



template <typename List, typename Default> struct SelectHeadIfNotEmpty;

template <typename Head, typename ... Tail, typename Default>
struct SelectHeadIfNotEmpty<TypeList<Head, Tail...>, Default> {
    typedef Head                                                                Result;
};

template <typename Default>
struct SelectHeadIfNotEmpty<TypeList<>, Default> {
    typedef Default                                                             Result;
};






template <bool Value, typename ResultIfTrue, typename Else>
struct IfThenElseT {
    typedef typename Select<
                Value ? 0 : 1,
                TypeList<ResultIfTrue, Else>
    >::Result                                                                   Result;
};

template <bool Value, typename ResultIfTrue, typename Else>
using IfThenElse = Select<
        Value ? 0 : 1,
        TypeList<ResultIfTrue, Else>
>;


template <typename Type1, typename Type2>
struct IfTypesEqual {
    static const bool Value = false;
};

template <typename Type>
struct IfTypesEqual<Type, Type> {
    static const bool Value = true;
};

}
