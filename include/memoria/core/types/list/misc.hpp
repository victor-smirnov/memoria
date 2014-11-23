
// Copyright Victor Smirnov 2011-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_MISC_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_MISC_HPP

#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/list/asserts.hpp>
#include <memoria/core/types/types.hpp>

namespace memoria    {


template <typename List, typename Default> struct SelectHeadIfNotEmpty;

template <typename Head, typename ... Tail, typename Default>
struct SelectHeadIfNotEmpty<TypeList<Head, Tail...>, Default> {
    typedef Head                                                                Result;
};

template <typename Default>
struct SelectHeadIfNotEmpty<TypeList<>, Default> {
    typedef Default                                                             Result;
};

template<typename...> struct False {
	static const bool Value = false;
};

template <typename List> struct ListSize {
	static_assert(False<List>::Value, "Type supplied to ListSize<> template is not allowed");
};

//template <typename List> struct ListSize;

template <typename ... List>
struct ListSize<TypeList<List...> > {
    static const Int Value = sizeof...(List);
};

template <typename T, T ... List>
struct ListSize<ValueList<T, List...> > {
    static const Int Value = sizeof...(List);
};



template <typename List> struct IsPlainList;

template <typename H, typename... Tail>
struct IsPlainList<TypeList<H, Tail...>> {
	static const bool Value = (!IsList<H>::Value) && IsPlainList<TypeList<Tail...>>::Value;
};

template <>
struct IsPlainList<TypeList<>> {
	static const bool Value = true;
};



}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_TYPECHAIN_HPP */
