
// Copyright 2016 Victor Smirnov
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
#include <memoria/core/tools/static_array.hpp>

namespace memoria {
namespace btfl {

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
class StreamRankName        {};
class StreamSumsName        {};

//class BranchCommonName      {};
//class LeafCommonName        {};
//class BranchFixedName       {};
//class LeafFixedName         {};
//class BranchVariableName    {};
//class LeafVariableName      {};


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
class MiscName              {};
class RanksName             {};



class IteratorToolsName     {};
class IteratorAPIName       {};
class IteratorMultiskipName {};
class IteratorContainerAPIName  {};
class IteratorFindName      {};
class IteratorSelectName    {};
class IteratorRankName      {};
class IteratorSkipName      {};
class IteratorLeafName      {};
class IteratorMiscName      {};
class IteratorStreamRankName {};
class IteratorStreamSumsName {};
class IteratorUpdateName    {};
class IteratorRemoveName    {};
class IteratorInsertName    {};
class IteratorReadName      {};
class IteratorBasicName     {};

}

}
