
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/container/container.hpp>

namespace memoria {

template <typename Profile> class ContainerCollectionCfg;

namespace bt {

template <typename ContainerName>
class ContainerPartsListFactory     {};

template <typename ContainerName>
class IteratorPartsListFactory      {};

class ToolsName             {};
class ToolsPLName           {};
class FindName              {};
class IOReadName            {};
class ReadName              {};
class UpdateName            {};
class CoWOpsName            {};
class NoCoWOpsName          {};


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

class NodeCommonName        {};

class ApiName               {};
class ChecksName            {};
class WalkName              {};
class BlockName             {};




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
class IteratorCountName     {};
class IteratorRankName      {};
class IteratorSkipName      {};
class IteratorLeafName      {};

}

template <typename Types>
struct BTCtrTypesT: CtrTypesT<Types>     {};

template <typename Types>
struct BTIterTypesT: IterTypesT<Types>   {};

template <typename Types>
struct BTBlockIterStateTypesT: BlockIterStateTypesT<Types>   {};



template <typename Types>
using BTCtrTypes = BTCtrTypesT<Types>;

template <typename Types>
using BTIterTypes = BTIterTypesT<Types>;

template <typename Types>
using BTBlockIterStateTypes = BTBlockIterStateTypesT<Types>;

}
