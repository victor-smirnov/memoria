
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TYPES_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TYPES_HPP

#include <memoria/core/container/container.hpp>



namespace memoria    {

template <typename Profile> class ContainerCollectionCfg;
template <typename> 		class PackedFSETree;
template <typename> 		class PackedFSEArray;

namespace balanced_tree     {

class IDType {};

template <
	template <typename> class NonLeafPackedTypeFactory,
	template <typename> class LeafPackedTypeFactory,
	Int Indexes_
>
struct StreamDescr {
	static const Int Indexes = Indexes_;
	typedef BigInt IndexType;
};




template <typename Types>
struct PackedFSETreeTF {
	typedef PackedFSETree<Types> Type;
};

template <typename Types>
struct PackedFSEArrayTF {
	typedef PackedFSEArray<Types> Type;
};

template <typename ContainerName>
class ContainerPartsListFactory     {};

template <typename ContainerName>
class IteratorPartsListFactory      {};

class ToolsName             {};
class FindName              {};
class InsertName            {};
class InsertBatchName       {};
class RemoveName            {};
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
class IteratorFindName		{};

}

template <typename Types>
struct BalTreeCtrTypesT: CtrTypesT<Types>         {};

template <typename Types>
struct BalTreeIterTypesT: IterTypesT<Types>   {};


template <typename Types>
using BalTreeCtrTypes = BalTreeCtrTypesT<Types>;

template <typename Types>
using BalTreeIterTypes = BalTreeIterTypesT<Types>;


}


#endif  // _MEMORIA_PROTOTYPES_BALANCEDTREE_NAMES_HPP

