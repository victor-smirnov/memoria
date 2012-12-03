
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_ASSERTS_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_ASSERTS_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/list/typelist.hpp>

namespace memoria    {

template <typename Type>
struct IsList: ConstValue<bool, false> {};

template <typename ... List>
struct IsList<TypeList<List...> >: ConstValue<bool, true> {};


template <typename Type>
struct IsNonemptyList: ConstValue<bool, false> {};

template <typename ... List>
struct IsNonemptyList<TypeList<List...>>: ConstValue<bool, true> {};

template <>
struct IsNonemptyList<TypeList<>>: ConstValue<bool, false> {};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_TYPELIST_HPP */
