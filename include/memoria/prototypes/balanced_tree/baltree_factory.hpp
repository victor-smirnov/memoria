
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_FACTORY_HPP

#include <memoria/core/types/type2type.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/vector_tuple.hpp>

#include <memoria/prototypes/balanced_tree/baltree_types.hpp>
#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/prototypes/balanced_tree/baltree_walkers.hpp>

#include <memoria/prototypes/balanced_tree/nodes/tree_node.hpp>
#include <memoria/prototypes/balanced_tree/nodes/leaf_node.hpp>
#include <memoria/prototypes/balanced_tree/nodes/node_dispatcher.hpp>
#include <memoria/prototypes/balanced_tree/nodes/tree_metadata.hpp>
#include <memoria/prototypes/balanced_tree/nodes/node_list_builder.hpp>

#include <memoria/prototypes/balanced_tree/container/baltree_c_base.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_tools.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_checks.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_instools.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_insbatch.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_insert.hpp>

#include <memoria/prototypes/balanced_tree/container/baltree_c_remove.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_remtools.hpp>
#include <memoria/prototypes/balanced_tree/container/baltree_c_rembatch.hpp>
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

    typedef TypeList<
            memoria::btree::AllocatorName,
            memoria::balanced_tree::ToolsName,
            memoria::balanced_tree::ChecksName,
            memoria::balanced_tree::InsertBatchName,
            memoria::balanced_tree::InsertName,
            memoria::balanced_tree::InsertToolsName,
            memoria::balanced_tree::RemoveToolsName,
            memoria::balanced_tree::RemoveBatchName,
            memoria::balanced_tree::RemoveName,
            memoria::balanced_tree::FindName
    >                                                                           ContainerPartsList;
    
    typedef TypeList<
            memoria::balanced_tree::IteratorAPIName
    >                                                                           IteratorPartsList;

    typedef EmptyType                                                           ContainerInterface;
    typedef EmptyType                                                           IteratorInterface;
    typedef EmptyType                                                           IteratorData;


    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;
    typedef typename Allocator::ID                                              ID;

//    typedef BalancedTreeMetadata<ID>                                            Metadata;

    typedef TypeList<
//    		AllNodeTypes<balanced_tree::TreeMapNode>
    >																			NodeTypesList;

    typedef TypeList<
//    		LeafNodeType<TreeMapNode>,
//    		InternalNodeType<TreeMapNode>,
//    		RootNodeType<TreeMapNode>,
//    		RootLeafNodeType<TreeMapNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
//    		StreamDescr<1>
    >																			StreamDescriptors;


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

    static const Int MAIN_STREAM												= 0;


    template <typename Types>
    using SkipForwardWalker = TypeIsNotDefined;

    template <typename Types>
    using SkipBackwardWalker = TypeIsNotDefined;

    template <typename Types>
    using NextLeafWalker = TypeIsNotDefined;

    template <typename Types>
    using PrevLeafWalker = TypeIsNotDefined;


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

    static const Int Streams = ListSize<typename ContainerTypes::StreamDescriptors>::Value;

    typedef typename balanced_tree::AccumulatorBuilder<
    			typename balanced_tree::AccumulatorListBuilder<
    				typename ContainerTypes::StreamDescriptors
    			>::Type
    >::Type																		Accumulator_;

    typedef balanced_tree::StaticVector<
    			BigInt,
    			Streams
    >       																	Position_;


    typedef balanced_tree::TreeNodeBase<typename ContainerTypes::Allocator::Page>   NodePageBase0;
    typedef PageGuard<NodePageBase0, typename ContainerTypes::Allocator>   		NodePageBase0G;

    struct NodeTypes: ContainerTypes {
        typedef NodePageBase0                      			NodePageBase;
        typedef ContainerTypeName_                          Name;
        typedef typename ContainerTypes::Metadata			Metadata;

        typedef typename ListHead<typename ContainerTypes::KeysList>::Type		Key;
        typedef typename MyType::Value                      Value;
        typedef typename MyType::ID							ID;

        typedef typename ContainerTypes::StreamDescriptors	StreamDescriptors;

        static const Int                                    Indexes             = 1;
        static const Int                                    Streams				= MyType::Streams;

        typedef Accumulator_ 								Accumulator;
        typedef Position_ 									Position;
    };

    struct DispatcherTypes
    {
        typedef typename ContainerTypes::NodeTypesList      NodeList;
        typedef typename ContainerTypes::DefaultNodeTypesList DefaultNodeList;
        typedef NodePageBase0                          		NodeBase;
        typedef typename MyType::NodeTypes					NodeTypes;
        typedef NodePageBase0G                         		NodeBaseG;
        typedef typename ContainerTypes::Allocator          Allocator;
    };

    typedef balanced_tree::BTreeDispatchers2<DispatcherTypes>                    PageDispatchers;


public:
    struct Types: ContainerTypes
    {
        typedef ContainerTypeName_                                              ContainerTypeName;
        typedef typename MyType::Value                                          Value;
        typedef typename MyType::PageDispatchers                                Pages;

        typedef typename ContainerTypes::Allocator                              Allocator;
        typedef typename ContainerTypes::Metadata                               Metadata;

        typedef NodePageBase0                                              		NodeBase;
        typedef NodePageBase0G                                             		NodeBaseG;

        typedef TypeList<>                                                      EmbeddedContainersList;

        typedef typename ContainerTypes::ContainerPartsList                     CtrList;
        typedef typename ContainerTypes::IteratorPartsList                      IterList;

        // FIXME Refactor BTree hierarchy
        // Use container types as base definitions
        typedef BalTreeCtrTypes<Types>                                          CtrTypes;
        typedef BalTreeIterTypes<Types>                                         IterTypes;

        typedef balanced_tree::NodePath<
                NodeBaseG, 8
        >                                                                       TreePath;

        typedef typename TreePath::PathItem                                     TreePathItem;

        typedef typename MaxElement<
                typename ContainerTypes::KeysList, TypeSizeValueProvider
        >::Result                                                               Key;


        static const Int Streams												= MyType::Streams;
        typedef Accumulator_       												Accumulator;
        typedef balanced_tree::StaticVector<BigInt, MyType::Streams>       		Position;



        typedef ValuePair<Accumulator, Value>                                   Element;

        typedef IDataSource<Value>                                     			IDataSourceType;
        typedef IDataTarget<Value>                                     			IDataTargetType;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;
};



}

#endif
