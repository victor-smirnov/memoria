
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_NAMES_HPP
#define _MEMORIA_MODELS_ARRAY_NAMES_HPP

#include <memoria/prototypes/dynvector/names.hpp>


namespace memoria       {
namespace mvector       {

using namespace memoria::dynvector;

class ApiName               	{};
class ContainerApiName          {};
class IteratorToolsName     	{};
class IteratorContainerAPIName  {};

}

template <typename Types>
struct VectorCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct VectorIterTypesT: IterTypesT<Types> {};

template <typename Types>
using VectorCtrTypes  = DynVectorCtrTypes<VectorCtrTypesT<Types>>;

template <typename Types>
using VectorIterTypes = DynVectorIterTypes<VectorIterTypesT<Types>>;

}

#endif
