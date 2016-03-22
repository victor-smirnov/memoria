
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/prototypes/ctr_wrapper/ctrwrapper_names.hpp>

namespace memoria    {
namespace louds      {

class CtrApiName        {};
class CtrFindName       {};
class CtrInsertName     {};
class CtrUpdateName     {};
class CtrRemoveName     {};
class CtrChecksName     {};

class ItrApiName        {};

}

template <typename Types>
struct LoudsCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct LoudsIterTypesT: IterTypesT<Types> {};



template <typename Types>
using LoudsCtrTypes  = CtrWrapperTypes<LoudsCtrTypesT<Types>>;

template <typename Types>
using LoudsIterTypes = IterWrapperTypes<LoudsIterTypesT<Types>>;




}
