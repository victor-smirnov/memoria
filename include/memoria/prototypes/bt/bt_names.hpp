
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TYPES_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TYPES_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/tools/static_array.hpp>

namespace memoria    {

template <typename Profile> class ContainerCollectionCfg;

namespace bt     {

template <typename ContainerName>
class ContainerPartsListFactory     {};

template <typename ContainerName>
class IteratorPartsListFactory      {};

class ToolsName             {};
class FindName              {};
class ReadName              {};
class UpdateName            {};

class InsertName            {};
class InsertBatchName       {};
class InsertToolsName       {};

class RemoveName            {};
class RemoveToolsName       {};
class RemoveBatchName       {};

class NodeNormName          {};
class NodeComprName         {};

class ApiName               {};
class ChecksName            {};
class WalkName              {};
class AllocatorName         {};



template <typename MyType, typename TypesType, typename ContainerTypeName>
class BTreeIteratorBaseFactoryName  {};

template <typename ContainerType, typename ContainerTypeName>
class BTreeIteratorFactoryName      {};

class IteratorToolsName     {};
class IteratorAPIName       {};
class IteratorMultiskipName {};
class IteratorContainerAPIName  {};
class IteratorFindName      {};

}

template <typename Types>
struct BTCtrTypesT: CtrTypesT<Types>         {};

template <typename Types>
struct BTIterTypesT: IterTypesT<Types>   {};


template <typename Types>
using BTCtrTypes = BTCtrTypesT<Types>;

template <typename Types>
using BTIterTypes = BTIterTypesT<Types>;


}


#endif  // _MEMORIA_PROTOTYPES_BALANCEDTREE_NAMES_HPP

