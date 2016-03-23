
// Copyright Victor Smirnov 2011-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/types/list/typelist.hpp>

namespace memoria {
namespace v1 {

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

}}