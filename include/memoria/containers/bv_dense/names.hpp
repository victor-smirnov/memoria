
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_BV_DENSE_NAMES_HPP
#define _MEMORIA_CONTAINERS_BV_DENSE_NAMES_HPP

#include <memoria/containers/vector/names.hpp>

namespace memoria    	{
namespace bv_dense		{

class CtrInsertName {};
class CtrRemoveName {};
class CtrToolsName  {};
class CtrAPIName  	{};

class IterAPIName  	{};

}

template <typename Types>
struct BitVectorCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct BitVectorIterTypesT: IterTypesT<Types> {};

template <typename Types>
using BitVectorCtrTypes  = VectorCtrTypes<BitVectorCtrTypesT<Types>>;

template <typename Types>
using BitVectorIterTypes = VectorIterTypes<BitVectorIterTypesT<Types>>;

}

#endif
