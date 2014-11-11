
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_TYPELIST_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_TYPELIST_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/tools/type_name.hpp>

#include <ostream>

namespace memoria    {

template <typename ... List> struct ListHead;

template <typename Head, typename ... Tail>
struct ListHead<TypeList<Head, Tail...>> {
    typedef Head Type;
};

template <typename T, T Head, T ... Tail>
struct ListHead<ValueList<T, Head, Tail...>> {
    static const T Value = Head;
};


template <typename ... List> struct ListTail;

template <typename Head, typename ... Tail>
struct ListTail<TypeList<Head, Tail...>> {
    typedef TypeList<Tail...> Type;
};

template <typename T, T Head, T ... Tail>
struct ListTail<ValueList<T, Head, Tail...>> {
    typedef ValueList<T, Tail...> Type;
};

template <typename T> struct ListPrinter;

template <typename Head, typename... Tail>
struct ListPrinter<TypeList<Head, Tail...>> {
	static void print(std::ostream& out)
	{
		out<<::memoria::vapi::TypeNameFactory<Head>::name()<<std::endl;
		ListPrinter<TypeList<Tail...>>::print(out);
	}
};

template <>
struct ListPrinter<TypeList<>> {
	static void print(std::ostream& out)
	{
	}
};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_TYPELIST_HPP */
