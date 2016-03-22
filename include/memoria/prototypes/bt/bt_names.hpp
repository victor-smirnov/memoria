
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/container/container.hpp>

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
class InsertBatchVariableName {};
class InsertBatchFixedName  {};
class InsertBatchCommonName {};
class InsertToolsName       {};

class RemoveName            {};
class RemoveToolsName       {};
class RemoveBatchName       {};

class BranchCommonName      {};
class BranchFixedName       {};
class BranchVariableName    {};

class LeafCommonName        {};
class LeafFixedName         {};
class LeafVariableName      {};

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
class IteratorSelectName    {};
class IteratorRankName      {};
class IteratorSkipName      {};
class IteratorLeafName      {};

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
