
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/bt_names.hpp>

namespace memoria    {
namespace map        {

class CtrApiName        {};
class CtrCApiName       {};
class CtrInsertName     {};
class CtrInsertMaxName  {};
class CtrCInsertName    {};
class CtrCNavName       {};
class CtrRemoveName     {};
class CtrCRemoveName    {};


class ItrApiName {};
class ItrCApiName {};
class ItrNavName {};
class ItrNavMaxName {};
class ItrCNavName {};

class ItrValueName {};
class ItrCValueName {};
class ItrMrkValueName {};

}

template <typename Types>
struct MapCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct MapIterTypesT: IterTypesT<Types> {};



template <typename Types>
using MapCtrTypes  = BTCtrTypes<MapCtrTypesT<Types>>;

template <typename Types>
using MapIterTypes = BTIterTypes<MapIterTypesT<Types>>;


}
