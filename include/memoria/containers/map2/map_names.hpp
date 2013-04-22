
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP2_NAMES_HPP
#define _MEMORIA_MODELS_IDX_MAP2_NAMES_HPP

#include <memoria/prototypes/ctr_wrapper/ctrwrapper_names.hpp>

namespace memoria    {
namespace map2 		 {

class CtrApiName 	{};
class CtrInsertName {};
class CtrToolsName 	{};
class CtrRemoveName {};

class ItrApiName {};
class ItrNavName {};


}

template <typename Types>
struct Map2CtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct Map2IterTypesT: IterTypesT<Types> {};



template <typename Types>
using Map2CtrTypes  = Map2CtrTypesT<Types>;

template <typename Types>
using Map2IterTypes = Map2IterTypesT<Types>;


}

#endif
