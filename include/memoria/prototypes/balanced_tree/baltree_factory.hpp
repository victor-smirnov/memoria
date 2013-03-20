
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_FACTORY_HPP

#include <memoria/core/types/type2type.hpp>
#include <memoria/prototypes/balanced_tree/baltree_names.hpp>
#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/prototypes/balanced_tree/baltree_walkers.hpp>

#include <memoria/prototypes/balanced_tree/pages/pages.hpp>

#include <memoria/prototypes/balanced_tree/container/baltree_c_base.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_tools.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_checks.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_insert.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_remove.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_find.hpp>

#include <memoria/prototypes/templates/container/allocator.hpp>

#include <memoria/prototypes/balanced_tree/baltree_iterator.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>


#include <memoria/prototypes/btree/names.hpp>

namespace memoria    {

using namespace memoria::balanced_tree;

template <typename Profile_, typename ContainerTypeSelector>
struct BalancedTreeTypes {

    typedef Profile_                                                            Profile;

    typedef TypeList<BigInt>                                                    KeysList;

    static const Int Indexes                                                    = 1;

    typedef TypeList<
            memoria::btree::AllocatorName,
            memoria::balanced_tree::ToolsName,
            memoria::balanced_tree::ChecksName,
            memoria::balanced_tree::InsertBatchName,
            memoria::balanced_tree::RemoveName,
            memoria::balanced_tree::FindName
    >                                                                           ContainerPartsList;

    typedef TypeList<>                                                          BasePagePartsList;
    
    typedef TypeList<>                                                          RootPagePartsList;

    typedef TypeList<>                                                          InternalPagePartsList;

    typedef TypeList<>                                                          LeafPagePartsList;

    typedef TypeList<>                                                          DataPagesList;

    typedef TypeList<
            memoria::balanced_tree::IteratorAPIName
    >                                                                           IteratorPartsList;

    typedef EmptyType                                                           ContainerInterface;
    typedef EmptyType                                                           IteratorInterface;
    typedef EmptyType                                                           IteratorData;


    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;
    typedef typename Allocator::ID                                              ID;

    typedef BTreeMetadata<ID>                                                   Metadata;


    template <
        typename Types_
    >
    struct CtrBaseFactory {
        typedef balanced_tree::BTreeContainerBase<Types_>                       Type;
    };

    template <
        typename Types_
    >
    struct IterBaseFactory {
        typedef BalTreeIteratorBase<Types_>                      				Type;
    };


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef balanced_tree::BTreeIteratorCache<Iterator, Container>          Type;
    };


    template <typename Types>
    using FindBeginWalker = TypeIsNotDefined;

    template <typename Types>
    using FindEndWalker = TypeIsNotDefined;

    template <typename Types>
    using FindRBeginWalker = TypeIsNotDefined;

    template <typename Types>
    using FindREndWalker = TypeIsNotDefined;
};


template <
        typename Profile,
        typename ContainerTypeName_
>
class CtrTF<Profile, memoria::BalancedTree, ContainerTypeName_> {

    typedef CtrTF<Profile, memoria::BalancedTree, ContainerTypeName_>           MyType;

public:


    typedef BalancedTreeTypes<Profile, ContainerTypeName_>                      ContainerTypes;


    typedef typename AppendTool<
    			balanced_tree::RootNodeMetadataName<typename ContainerTypes::Metadata>,
                typename ContainerTypes::RootPagePartsList
    >::Result                                                                   RootPagePartsList;
    
    typedef typename ContainerTypes::Allocator::Page::ID                        ID;

    //TAGS: #IF_THEN_ELSE_EXAMPLE
    typedef typename memoria::IfThenElse<
                IfTypesEqual<
                    typename ContainerTypes::Value,
                    memoria::balanced_tree::IDType
                >::Value,
                ID,
                typename ContainerTypes::Value
    >::Result                                                                   Value;


    struct BasePartsTypes{
        typedef balanced_tree::TreePage<typename ContainerTypes::Allocator::Page>      NodePageBase;
        typedef typename ContainerTypes::BasePagePartsList              List;
    };

    typedef PageStart<BasePartsTypes>                                           BasePageParts;
    typedef balanced_tree::NodePageBase<BasePageParts>                          NodePageBase0;

    struct NodePageContainerTypes: public NodePageBase0 {};


    struct NodeTypesBase {
        typedef NodePageContainerTypes                      NodePageBase;
        typedef ContainerTypeName_                          Name;
        static const Int                                    Indexes             = ContainerTypes::Indexes;
        typedef typename ContainerTypes::BasePagePartsList  BasePartsList;
    };


    typedef typename ListHead<typename ContainerTypes::KeysList>::Type          NodeKey;

    template <int Level> struct RootLeafTypes: NodeTypesBase {
        typedef NodeKey                                                     Key;
        typedef typename MyType::Value                                      Value;
        typedef typename AppendTool<
                RootPagePartsList,
                typename ContainerTypes::LeafPagePartsList
        >::Result                                                           List;
        typedef balanced_tree::NodeDescriptor<true, true, Level>            Descriptor;
    };

    template <int Level> struct LeafTypes: NodeTypesBase {
        typedef NodeKey                                                     Key;
        typedef typename MyType::Value                                      Value;
        typedef typename ContainerTypes::LeafPagePartsList                  List;
        typedef balanced_tree::NodeDescriptor<false, true, Level>           Descriptor;
    };

    template <int Level> struct RootTypes: NodeTypesBase {
        typedef NodeKey                                                     Key;
        typedef ID                                                          Value;
        typedef typename AppendTool<
                RootPagePartsList,
                typename ContainerTypes::InternalPagePartsList
        >::Result                                                           List;
        typedef balanced_tree::NodeDescriptor<true, false, Level>           Descriptor;
    };

    template <int Level> struct InternalTypes: NodeTypesBase {
        typedef NodeKey                                                     Key;
        typedef ID                                                          Value;
        typedef typename ContainerTypes::InternalPagePartsList              List;
        typedef balanced_tree::NodeDescriptor<false, false, Level>          Descriptor;
    };

    typedef typename balanced_tree::NodeTLBuilder <
                MyType,
                typename ContainerTypes::KeysList
    >::List                                                                     NodeTypesList;

    MEMORIA_STATIC_ASSERT(IsNonemptyList<NodeTypesList>::Value);


    typedef NodePageContainerTypes                                              NodeContainerTypes;
    typedef PageGuard<NodeContainerTypes, typename ContainerTypes::Allocator>   NodeContainerTypesG;

    struct DispatcherTypes
    {
        typedef NodeTypesList                               NodeList;
        typedef NodeContainerTypes                          NodeBase;
        typedef NodeContainerTypesG                         NodeBaseG;
        typedef typename ContainerTypes::Allocator          Allocator;
    };

    typedef balanced_tree::BTreeDispatchers<DispatcherTypes>                    PageDispatchers;


public:
    struct Types: ContainerTypes
    {
        typedef ContainerTypeName_                                              ContainerTypeName;
        typedef typename MyType::Value                                          Value;
        typedef typename MyType::PageDispatchers                                Pages;

        typedef typename ContainerTypes::Allocator                              Allocator;
        typedef typename ContainerTypes::Metadata                               Metadata;

        typedef NodeContainerTypes                                              NodeBase;
        typedef NodeContainerTypesG                                             NodeBaseG;


        typedef TypeList<>                                                      EmbeddedContainersList;



        typedef typename ContainerTypes::ContainerPartsList                     CtrList;
        typedef typename ContainerTypes::IteratorPartsList                      IterList;

        // FIXME Refactor BTree hierarchy
        // Use container types as base definitions
        typedef BalTreeCtrTypes<Types>                                            CtrTypes;
        typedef BalTreeIterTypes<Types>                                           IterTypes;

        typedef balanced_tree::NodePath<
                NodeBaseG, 8
        >                                                                       TreePath;

        typedef typename TreePath::PathItem                                     TreePathItem;

        typedef typename MaxElement<
                typename ContainerTypes::KeysList, TypeSizeValueProvider
        >::Result                                                               Key;

        typedef Accumulators<Key, ContainerTypes::Indexes>                      Accumulator;


        typedef ValuePair<Accumulator, Value>                                   Element;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;
};



}

#endif
