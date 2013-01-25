
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_VECTOR_NAMES_HPP
#define _MEMORIA_CONTAINERS_VECTOR_NAMES_HPP

#include <memoria/prototypes/sequence/names.hpp>


namespace memoria       {
namespace mvector       {

class IteratorContainerAPIName  {};

}

template <typename Types>
struct VectorCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct VectorIterTypesT: IterTypesT<Types> {};

template <typename Types>
using VectorCtrTypes  = SequenceCtrTypes<VectorCtrTypesT<Types>>;

template <typename Types>
using VectorIterTypes = SequenceIterTypes<VectorIterTypesT<Types>>;

}

#endif
