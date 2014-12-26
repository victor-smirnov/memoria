
// Copyright Victor Smirnov 2011-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_ALGO_SELECT_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_ALGO_SELECT_HPP

#include <memoria/core/types/list/typelist.hpp>
#include <memoria/core/types/list/index.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/types.hpp>



namespace memoria    {

template <Int Value, typename List, Int idx = 0> struct Select;

template <Int Value, typename Head, typename ... Tail>
struct Select<Value, TypeList<Head, Tail...>, Value> {
    typedef Head                                                                Result;
};

template <Int Value, typename Head, typename ... Tail, Int Idx>
struct Select<Value, TypeList<Head, Tail...>, Idx> {
    typedef typename Select<Value, TypeList<Tail...>, Idx + 1>::Result          Result;
};

template <Int Value, Int Idx>
struct Select<Value, TypeList<>, Idx>;
//{
//    typedef ListIndexOutOfRange<Value>                                          Result;
//};


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
struct IfThenElse {
    typedef typename Select<
                Value ? 0 : 1,
                TypeList<ResultIfTrue, Else>
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
