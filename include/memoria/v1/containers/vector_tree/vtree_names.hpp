
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/ctr_wrapper/ctrwrapper_names.hpp>

namespace memoria    {
namespace vtree      {

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
struct VTreeCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct VTreeIterTypesT: IterTypesT<Types> {};



template <typename Types>
using VTreeCtrTypes  = VTreeCtrTypesT<Types>;

template <typename Types>
using VTreeIterTypes = VTreeIterTypesT<Types>;


}
