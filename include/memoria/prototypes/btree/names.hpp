
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_NAMES_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_NAMES_HPP

namespace memoria    {


template <typename Profile, typename Params> class BTreeRootMetadataTypeFactory;
template <typename Profile, typename Params> class BTreeCountersTypeFactory;
template <typename Profile> class ContainerCollectionCfg;



namespace btree     {


template <typename ContainerTypes>
struct BTree0 {
    typedef ContainerTypes                                                          Types;
};

struct MapTypes {
    enum {Value, Index};
};

class IDType {};

template <typename ContainerName, typename BasePagesPartsList>
class NodeBaseFactory               {};

template <typename ContainerTypeName>
class BTreeCountersFactory          {};

template <typename ContainerTypeName>
class BTreeRootMetadataFactory      {
    typedef ContainerTypeName ContainerName;
};


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


class InitName              {};
class ToolsName             {};
class FindName              {};
class InsertName            {};
class RemoveName            {};
class MapApiName            {};
class ChecksName            {};
class StubsName             {};


template <typename MyType, typename TypesType, typename ContainerTypeName>
class BTreeIteratorBaseFactoryName  {};

template <typename ContainerType, typename ContainerTypeName>
class BTreeIteratorFactoryName      {};

class IteratorToolsName     {};
class IteratorWalkName      {};
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
}


#endif	// _MEMORIA_PROTOTYPES_BTREE_NAMES_HPP

