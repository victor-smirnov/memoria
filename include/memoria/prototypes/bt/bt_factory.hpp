
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_FACTORY_HPP

#include <memoria/core/types/type2type.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/vector_tuple.hpp>

#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/prototypes/bt/nodes/node_dispatcher.hpp>
#include <memoria/prototypes/bt/nodes/tree_metadata.hpp>
#include <memoria/prototypes/bt/nodes/node_list_builder.hpp>

#include <memoria/prototypes/bt/container/bt_c_base.hpp>
#include <memoria/prototypes/bt/container/bt_c_tools.hpp>
#include <memoria/prototypes/bt/container/bt_c_checks.hpp>
#include <memoria/prototypes/bt/container/bt_c_insbatch.hpp>
#include <memoria/prototypes/bt/container/bt_c_insert.hpp>
#include <memoria/prototypes/bt/container/bt_c_read.hpp>
#include <memoria/prototypes/bt/container/bt_c_update.hpp>
#include <memoria/prototypes/bt/container/bt_c_nodecompr.hpp>
#include <memoria/prototypes/bt/container/bt_c_nodenorm.hpp>
#include <memoria/prototypes/bt/container/bt_c_remtools.hpp>
#include <memoria/prototypes/bt/container/bt_c_rembatch.hpp>
#include <memoria/prototypes/bt/container/bt_c_find.hpp>
#include <memoria/prototypes/bt/container/bt_c_walk.hpp>
#include <memoria/prototypes/bt/container/bt_c_allocator.hpp>

#include <memoria/prototypes/bt/bt_iterator.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>


namespace memoria    {

template <typename Profile_, typename ContainerTypeSelector>
struct BTTypes {

    typedef Profile_                                                            Profile;

    using CtrSizeT = BigInt;

    typedef TypeList<
            bt::AllocatorName,
            bt::ToolsName,
            bt::ChecksName,
            bt::InsertBatchName,
            bt::InsertName,
            bt::RemoveToolsName,
            bt::RemoveBatchName,
            bt::FindName,
            bt::ReadName,
            bt::UpdateName,
            bt::WalkName
    >                                                                           ContainerPartsList;
    
    typedef TypeList<
            bt::IteratorAPIName,
            bt::IteratorFindName
    >                                                                           IteratorPartsList;

    typedef EmptyType                                                           ContainerInterface;
    typedef EmptyType                                                           IteratorInterface;
    typedef EmptyType                                                           IteratorData;


    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;
    typedef typename Allocator::ID                                              ID;

//  typedef TypeList<>                                                        	NodeTypesList;
//
//  typedef TypeList<>                                                        	DefaultNodeTypesList;

    typedef TypeList<
            NonLeafNodeTypes<BranchNode>,
            LeafNodeTypes<LeafNode>
    >                                                                           NodeTypesList;

    typedef TypeList<
            TreeNodeType<LeafNode>,
            TreeNodeType<BranchNode>
    >                                                                           DefaultNodeTypesList;


    typedef TypeList<
    >                                                                           StreamDescriptors;

    template <
        typename Types_
    >
    struct CtrBaseFactory {
        typedef bt::BTreeCtrBase<Types_>                    Type;
    };

    template <
        typename Types_
    >
    struct IterBaseFactory {
        typedef BTIteratorBase<Types_>                      Type;
    };


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef bt::BTreeIteratorCache<Iterator, Container> Type;
    };

    static const Int MAIN_STREAM                                                = 0;


    typedef EmptyType															DataSource;
    typedef EmptyType															DataTarget;


    template <typename Types>
    using SkipForwardWalker                 = TypeIsNotDefined;

    template <typename Types>
    using SkipBackwardWalker                = TypeIsNotDefined;

    template <typename Types>
    using NextLeafWalker                    = TypeIsNotDefined;

    template <typename Types>
    using PrevLeafWalker                    = TypeIsNotDefined;

    template <typename Types>
    using NextLeafMutistreamWalker          = TypeIsNotDefined;

    template <typename Types>
    using PrevLeafMutistreamWalker          = TypeIsNotDefined;




    template <typename Types>
    using FindBeginWalker                   = TypeIsNotDefined;

    template <typename Types>
    using FindEndWalker                     = TypeIsNotDefined;

    template <typename Types>
    using FindRBeginWalker                  = TypeIsNotDefined;

    template <typename Types>
    using FindREndWalker                    = TypeIsNotDefined;
};


template <
        typename Profile,
        typename ContainerTypeName_
>
class CtrTF<Profile, memoria::BT, ContainerTypeName_> {

    typedef CtrTF<Profile, memoria::BT, ContainerTypeName_>                     MyType;

public:


    typedef BTTypes<Profile, ContainerTypeName_>                                ContainerTypes;

    
    typedef typename ContainerTypes::Allocator::Page::ID                        ID;

    typedef typename ContainerTypes::Value										Value;

    static const Int Streams = ListSize<typename ContainerTypes::StreamDescriptors>::Value;

    typedef typename bt::TupleBuilder<
                typename bt::AccumulatorListBuilder<
                    typename ContainerTypes::StreamDescriptors
                >::Type
    >::Type                                                                     Accumulator_;

    typedef typename bt::TupleBuilder<
                typename bt::IteratorPrefixListBuilder<
                    typename ContainerTypes::StreamDescriptors
                >::Type
    >::Type                                                                     IteratorPrefix_;

    typedef core::StaticVector<
                typename ContainerTypes::CtrSizeT,
                Streams
    >                                                                           Position_;

    typedef typename ContainerTypes::Allocator::Page                            Page;

    typedef TreeNodeBase<
            typename ContainerTypes::Metadata,
            Page
    >                                                                           NodePageBase0;
    typedef PageGuard<NodePageBase0, typename ContainerTypes::Allocator>        NodePageBase0G;

    struct NodeTypes: ContainerTypes {
        typedef Page                                        NodeBase;
        typedef ContainerTypeName_                          Name;
        typedef typename ContainerTypes::Metadata           Metadata;

        typedef typename MyType::ID                         ID;

        typedef typename ContainerTypes::StreamDescriptors  StreamDescriptors;

        static const Int                                    Streams             = MyType::Streams;

        typedef Accumulator_                                Accumulator;
        typedef Position_                                   Position;
    };

    struct DispatcherTypes
    {
        typedef typename ContainerTypes::NodeTypesList      NodeList;
        typedef typename ContainerTypes::DefaultNodeTypesList DefaultNodeList;
        typedef NodePageBase0                               NodeBase;
        typedef typename MyType::NodeTypes                  NodeTypes;
        typedef NodePageBase0G                              NodeBaseG;
        typedef typename ContainerTypes::Allocator          Allocator;
    };

    typedef bt::BTreeDispatchers<DispatcherTypes>                              PageDispatchers;


public:
    struct Types: ContainerTypes
    {
        typedef ContainerTypeName_                                              ContainerTypeName;
        typedef typename MyType::PageDispatchers                                Pages;

        typedef typename ContainerTypes::Allocator                              Allocator;
        typedef typename ContainerTypes::Metadata                               Metadata;

        typedef NodePageBase0                                                   NodeBase;
        typedef NodePageBase0G                                                  NodeBaseG;

        typedef typename ContainerTypes::ContainerPartsList                     CtrList;
        typedef typename ContainerTypes::IteratorPartsList                      IterList;

        // FIXME Refactor BTree hierarchy
        // Use container types as base definitions
        typedef BTCtrTypes<Types>                                               CtrTypes;
        typedef BTIterTypes<Types>                                              IterTypes;

        static const Int Streams                                                = MyType::Streams;

        typedef Accumulator_                                                    Accumulator;
        typedef IteratorPrefix_                                                 IteratorPrefix;

        typedef Position_                     									Position;

        typedef PageUpdateManager<CtrTypes>                                     PageUpdateMgr;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;
};



}

#endif
