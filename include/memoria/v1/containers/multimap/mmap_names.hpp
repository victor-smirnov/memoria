
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/bt_names.hpp>

namespace memoria    {
namespace mmap       {

class CtrApiName        {};
class CtrCApiName       {};
class CtrInsertName     {};
class CtrCInsertName    {};
class CtrCNavName       {};
class CtrRemoveName     {};
class CtrCRemoveName    {};


class ItrApiName {};
class ItrCApiName {};
class ItrNavName {};
class ItrCNavName {};
class ItrMiscName {};

class ItrValueName {};
class ItrCValueName {};
class ItrMrkValueName {};

}

template <typename Types>
struct MultimapCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct MultimapIterTypesT: IterTypesT<Types> {};



template <typename Types>
using MultimapCtrTypes  = BTCtrTypes<MultimapCtrTypesT<Types>>;

template <typename Types>
using MultimapIterTypes = BTIterTypes<MultimapIterTypesT<Types>>;


}
