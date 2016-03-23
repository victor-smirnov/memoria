
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/ctr_wrapper/ctrwrapper_names.hpp>

namespace memoria {
namespace wt         {

template <typename... LabelDescriptors>
class WTLabeledTree {};

class CtrApiName    {};
class CtrCTreeName  {};
class CtrInsertName {};
class CtrToolsName  {};
class CtrRemoveName {};
class CtrChecksName {};
class CtrUpdateName {};
class CtrFindName {};

class ItrApiName {};
}

template <typename Types>
struct WTCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct WTIterTypesT: IterTypesT<Types> {};



template <typename Types>
using WTCtrTypes  = WTCtrTypesT<Types>;

template <typename Types>
using WTIterTypes = WTIterTypesT<Types>;




}
