
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_VCTR_NAMES_HPP
#define _MEMORIA_CONTAINERS_VCTR_NAMES_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria    {
namespace mvector    {

class CtrApiName    {};
class CtrInsertName {};
class CtrToolsName  {};
class CtrRemoveName {};
class CtrChecksName {};
class CtrFindName {};

class ItrApiName {};
}

template <typename Types>
struct Vector2CtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct Vector2IterTypesT: IterTypesT<Types> {};



template <typename Types>
using Vector2CtrTypes  = BTCtrTypesT<Vector2CtrTypesT<Types>>;

template <typename Types>
using Vector2IterTypes = BTIterTypesT<Vector2IterTypesT<Types>>;


}

#endif
