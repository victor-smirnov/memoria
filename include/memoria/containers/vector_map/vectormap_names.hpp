
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_VECTORMAP2_NAMES_HPP
#define _MEMORIA_CONTAINERS_VECTORMAP2_NAMES_HPP

#include <memoria/prototypes/ctr_wrapper/ctrwrapper_names.hpp>

namespace memoria    {
namespace vmap 	 	 {

class CtrApiName 	{};
class CtrInsertName {};
class CtrToolsName 	{};
class CtrRemoveName {};
class CtrChecksName {};
class CtrUpdateName {};
class CtrFindName {};

class ItrApiName {};
}

template <typename Types>
struct VectorMap2CtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct VectorMap2IterTypesT: IterTypesT<Types> {};



template <typename Types>
using VectorMap2CtrTypes  = VectorMap2CtrTypesT<Types>;

template <typename Types>
using VectorMap2IterTypes = VectorMap2IterTypesT<Types>;


}

#endif
