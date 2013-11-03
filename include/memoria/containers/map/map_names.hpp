
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_NAMES_HPP
#define _MEMORIA_MODELS_IDX_MAP_NAMES_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria    {
namespace map        {

class CtrApiName    {};
class CtrInsertName {};
class CtrRemoveName {};


class ItrApiName {};
class ItrNavName {};

class ItrValueName {};
class ItrMrkValueName {};

}

template <typename Types>
struct MapCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct MapIterTypesT: IterTypesT<Types> {};



template <typename Types>
using MapCtrTypes  = MapCtrTypesT<Types>;

template <typename Types>
using MapIterTypes = MapIterTypesT<Types>;


}

#endif
