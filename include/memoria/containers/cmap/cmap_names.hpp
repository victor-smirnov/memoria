
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_CMAP_NAMES_HPP
#define _MEMORIA_MODELS_IDX_CMAP_NAMES_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria    {
namespace cmap 		 {

class CtrApiName 	{};
class CtrInsertName {};
class CtrToolsName 	{};
class CtrRemoveName {};

class ItrApiName {};
class ItrNavName {};


}

template <typename Types>
struct CMap2CtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct CMap2IterTypesT: IterTypesT<Types> {};



template <typename Types>
using CMap2CtrTypes  = CMap2CtrTypesT<Types>;

template <typename Types>
using CMap2IterTypes = CMap2IterTypesT<Types>;


}

#endif
