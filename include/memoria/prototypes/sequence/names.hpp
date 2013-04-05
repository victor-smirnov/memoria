
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_NAMES_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_NAMES_HPP

#include <memoria/prototypes/balanced_tree/baltree_types.hpp>

namespace memoria    	{
namespace sequence		{

class CtrInsertName {};
class CtrRemoveName {};
class CtrToolsName  {};
class CtrFindName  	{};
class CtrChecksName {};

class IterAPIName  	{};

}

template <typename Types>
struct SequenceCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct SequenceIterTypesT: IterTypesT<Types> {};

template <typename Types>
using SequenceCtrTypes  = BalTreeCtrTypes<SequenceCtrTypesT<Types>>;

template <typename Types>
using SequenceIterTypes = BalTreeIterTypes<SequenceIterTypesT<Types>>;

}

#endif
