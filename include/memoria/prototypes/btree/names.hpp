
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_NAMES_HPP
#define _MEMORIA_PROTOTYPES_BTREE_NAMES_HPP

#include <memoria/core/container/container.hpp>

namespace memoria    {


//template <typename Profile, typename Params> class BTreeRootMetadataTypeFactory;
template <typename Profile> class ContainerCollectionCfg;



namespace btree     {


template <typename ContainerTypes>
struct BTree0 {
    typedef ContainerTypes                                                          Types;
};

struct MapTypes {
    enum {Value, Sum};
};

class IDType {};

template <typename ContainerName, typename BasePagesPartsList>
class NodeBaseFactory               {};

template <typename ContainerName>
class ContainerPartsListFactory     {};

template <typename ContainerName>
class IteratorPartsListFactory      {};



template <typename ContainerName>
class BasePagePartsListFactory      {};

template <typename ContainerName>
class RootPagePartsListFactory      {};

template <typename ContainerName>
class LeafPagePartsListFactory      {};

template <typename ContainerName>
class InternalPagePartsListFactory  {};


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

template <typename ContainerTypeName>
class RootNodeMetadataName  {
    typedef ContainerTypeName ContainerName;
};

template <
    typename BasePagePartsListType,
    typename RootPagePartsListType,
    typename InternalPagePartsListType,
    typename LeafPagePartsListType
>
struct Lists {
    typedef BasePagePartsListType                                               BasePagePartsList;
    typedef RootPagePartsListType                                               RootPagePartsList;
    typedef InternalPagePartsListType                                           InternalPagePartsList;
    typedef LeafPagePartsListType                                               LeafPagePartsList;
};


template <typename Name>
struct ValueTypeIsNotSupported;

}

template <typename Types>
struct BTreeCtrTypesT: CtrTypesT<Types>         {};

template <typename Types>
struct BTreeIterTypesT: IterTypesT<Types>   {};


template <typename Types>
using BTreeCtrTypes = BTreeCtrTypesT<Types>;

template <typename Types>
using BTreeIterTypes = BTreeIterTypesT<Types>;


}


#endif  // _MEMORIA_PROTOTYPES_BTREE_NAMES_HPP

