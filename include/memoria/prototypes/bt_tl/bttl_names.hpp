
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_NAMES_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_NAMES_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/tools/static_array.hpp>

namespace memoria    {
namespace bttl		 {

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

class LeafCommonName      	{};
class LeafFixedName       	{};
class LeafVariableName    	{};

class ApiName               {};
class ChecksName            {};
class WalkName              {};
class AllocatorName         {};
class MiscName         		{};



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


#endif  // _MEMORIA_PROTOTYPES_BALANCEDTREE_NAMES_HPP

