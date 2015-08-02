
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_TABLE_NAMES_HPP
#define _MEMORIA_CONTAINERS_TABLE_NAMES_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria    {
namespace table      {

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
struct TableCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct TableIterTypesT: IterTypesT<Types> {};



template <typename Types>
using TableCtrTypes  = BTCtrTypes<TableCtrTypesT<Types>>;

template <typename Types>
using TableIterTypes = BTIterTypes<TableIterTypesT<Types>>;


}

#endif
