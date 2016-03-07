
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_NAMES_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_NAMES_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria       {
namespace seq_dense     {

class CtrChecksName     {};
class CtrToolsName      {};
class CtrFindName       {};
class CtrInsertName     {};
class CtrInsertFixedName    {};
class CtrInsertVariableName {};
class CtrComprName      {};
class CtrNormName       {};
class CtrRemoveName     {};

class IterMiscName      {};
class IterRankName      {};
class IterSelectName    {};
class IterCountName     {};
class IterSkipName      {};

}

template <typename Types>
struct DenseSeqCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct DenseSeqIterTypesT: IterTypesT<Types> {};

template <typename Types>
using DenseSeqCtrTypes  = BTCtrTypes<DenseSeqCtrTypesT<Types>>;

template <typename Types>
using DenseSeqIterTypes = BTCtrTypes<DenseSeqIterTypesT<Types>>;

}

#endif
