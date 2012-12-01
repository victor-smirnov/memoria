
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_ALGO_SELECT_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_ALGO_SELECT_HPP

#include <memoria/core/types/list/typelist.hpp>
#include <memoria/core/types/list/index.hpp>
#include <memoria/core/types/types.hpp>



namespace memoria    {

template <Int Value, typename List, Int idx = 0> struct Select;

/*
template <Int Value, typename Head, typename Tail>
struct Select<Value, TL<Head, Tail>, Value> {
    typedef Head                                                                Result;
};

template <Int Value, typename Head, typename Tail, Int Idx>
struct Select<Value, TL<Head, Tail>, Idx> {
    typedef typename Select<Value, Tail, Idx + 1>::Result                       Result;
};

template <Int Value, Int Idx>
struct Select<Value, NullType, Idx> {
    typedef ListIndexOutOfRange<Value>                                          Result;
};
*/

template <Int Value, typename Head, typename ... Tail>
struct Select<Value, VTL<Head, Tail...>, Value> {
    typedef Head                                                                Result;
};

template <Int Value, typename Head, typename ... Tail, Int Idx>
struct Select<Value, VTL<Head, Tail...>, Idx> {
    typedef typename Select<Value, VTL<Tail...>, Idx + 1>::Result               Result;
};

template <Int Value, Int Idx>
struct Select<Value, VTL<>, Idx> {
    typedef ListIndexOutOfRange<Value>                                          Result;
};


template <bool Value, typename ResultIfTrue, typename Else>
struct IfThenElse {
    typedef typename Select<
                Value ? 0 : 1,
                VTL<ResultIfTrue, Else >
    >::Result                                                                   Result;
};


template <typename Type1, typename Type2>
struct IfTypesEqual {
    static const bool Value = false;
};

template <typename Type>
struct IfTypesEqual<Type, Type> {
    static const bool Value = true;
};

}

#endif
