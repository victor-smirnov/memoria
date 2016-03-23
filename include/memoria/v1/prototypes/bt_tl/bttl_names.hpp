
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

namespace memoria    {
namespace bttl       {

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
class IteratorStreamRankName{};
class IteratorUpdateName    {};
class IteratorRemoveName    {};
class IteratorInsertName    {};

}

template <typename Types>
struct TLCtrTypesT: BTCtrTypesT<Types>         {};

template <typename Types>
struct TLIterTypesT: BTIterTypesT<Types>   {};


template <typename Types>
using BTTLCtrTypes = BTCtrTypesT<TLCtrTypesT<Types>>;

template <typename Types>
using BTTLIterTypes = BTIterTypesT<TLIterTypesT<Types>>;


}
