
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_NAMES_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_NAMES_HPP

#include <memoria/prototypes/sequence/names.hpp>

namespace memoria    	{
namespace seq_dense		{

class CtrChecksName {};
class CtrToolsName {};
class CtrFindName {};
class CtrInsertName {};
class CtrRemoveName {};

class IterAPIName  {};

}

template <typename Types>
struct DenseSequenceCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct DenseSequenceIterTypesT: IterTypesT<Types> {};

template <typename Types>
using DenseSequenceCtrTypes  = BTCtrTypes<DenseSequenceCtrTypesT<Types>>;

template <typename Types>
using DenseSequenceIterTypes = BTCtrTypes<DenseSequenceIterTypesT<Types>>;

}

#endif
