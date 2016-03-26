
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

#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/tools/vector_tuple.hpp>

#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>

#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_walkers.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>

#include <memoria/v1/prototypes/bt/walkers/bt_skip_walkers.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_find_walkers.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_findmax_walkers.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_select_walkers.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_misc_walkers.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_leaf_walkers.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_rank_walkers.hpp>

#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/nodes/node_dispatcher.hpp>
#include <memoria/v1/prototypes/bt/nodes/tree_metadata.hpp>
#include <memoria/v1/prototypes/bt/nodes/node_list_builder.hpp>

#include <memoria/v1/prototypes/bt/container/bt_c_base.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_tools.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_checks.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_insert.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_read.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_update.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_branch_common.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_branch_variable.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_branch_fixed.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_leaf_common.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_leaf_variable.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_leaf_fixed.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_find.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_walk.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_allocator.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_remove.hpp>

#include <memoria/v1/prototypes/bt/bt_iterator.hpp>

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_insert_batch_common.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_insert_batch_fixed.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_insert_batch_variable.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_remove_batch.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_remove_tools.hpp>


namespace memoria {
namespace v1 {

using v1::bt::WalkerTypes;

template <typename Profile_, typename ContainerTypeSelector>
struct BTTypes {

    using Profile  = Profile_;

    using CtrSizeT = BigInt;

    typedef TypeList<
            bt::AllocatorName,
            bt::ToolsName,
            bt::ChecksName,
            bt::BranchCommonName,
            bt::InsertBatchCommonName,
            bt::LeafCommonName,
            bt::InsertName,
            bt::RemoveToolsName,
            bt::RemoveBatchName,
            bt::RemoveName,
            bt::FindName,
            bt::ReadName,
            bt::UpdateName,
            bt::WalkName
    >                                                                           ContainerPartsList;


    typedef TypeList<
            bt::BranchFixedName,
            bt::InsertBatchFixedName
    >                                                                           FixedBranchContainerPartsList;

    typedef TypeList<
            bt::BranchVariableName,
            bt::InsertBatchVariableName
    >                                                                           VariableBranchContainerPartsList;

    typedef TypeList<
            bt::LeafFixedName
    >                                                                           FixedLeafContainerPartsList;

    typedef TypeList<
            bt::LeafVariableName
    >                                                                           VariableLeafContainerPartsList;
    
    typedef TypeList<>                                                          CommonContainerPartsList;


    typedef TypeList<
            bt::IteratorAPIName,
            bt::IteratorFindName,
            bt::IteratorSelectName,
            bt::IteratorRankName,
            bt::IteratorSkipName,
            bt::IteratorLeafName
    >                                                                           IteratorPartsList;

    typedef EmptyType                                                           ContainerInterface;
    typedef EmptyType                                                           IteratorInterface;
    typedef EmptyType                                                           IteratorData;


    using Allocator = typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator;
    using ID        = typename Allocator::ID;

    using Metadata  = BalancedTreeMetadata<ID>;

    typedef TypeList<
            BranchNodeTypes<BranchNode>,
            LeafNodeTypes<LeafNode>
    >                                                                           NodeTypesList;

    typedef TypeList<
            TreeNodeType<BranchNode>
    >                                                                           DefaultBranchNodeTypesList;

    typedef TypeList<
            TreeNodeType<LeafNode>
    >                                                                           DefaultLeafNodeTypesList;
    //FIXME DefaultNodeTypesList is not used anymore

    typedef TypeList<>                                                          StreamDescriptors;

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
        typedef v1::bt::BTreeIteratorPrefixCache<Iterator, Container>   Type;
    };



    template <typename Types, typename LeafPath>
    using FindGTForwardWalker          = bt::FindGTForwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindMaxGTWalker              = bt::FindMaxGTWalker<WalkerTypes<Types, LeafPath>>;


    template <typename Types, typename LeafPath>
    using FindGTBackwardWalker         = bt::FindGTBackwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindGEForwardWalker          = bt::FindGEForwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindMaxGEWalker              = bt::FindMaxGEWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindGEBackwardWalker         = bt::FindGEBackwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SkipForwardWalker     = bt::SkipForwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SkipBackwardWalker    = bt::SkipBackwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SelectForwardWalker   = bt::SelectForwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SelectBackwardWalker  = bt::SelectBackwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using RankForwardWalker   = bt::RankForwardWalker<bt::RankWalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using RankBackwardWalker  = bt::RankBackwardWalker<bt::RankWalkerTypes<Types, LeafPath>>;

    template <typename Types>
    using NextLeafWalker      = bt::ForwardLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker      = bt::BackwardLeafWalker<Types>;

    template <typename Types, typename LeafPath>
    using NextLeafMutistreamWalker  = bt::SkipForwardWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using PrevLeafMutistreamWalker  = bt::SkipBackwardWalker<WalkerTypes<Types, LeafPath>>;
};


template <
        typename Profile,
        typename ContainerTypeName_
>
class CtrTF<Profile, v1::BT, ContainerTypeName_> {

    typedef CtrTF<Profile, v1::BT, ContainerTypeName_>                     MyType;

public:

    using ContainerTypes = BTTypes<Profile, ContainerTypeName_>;
    using ID             = typename ContainerTypes::Allocator::Page::ID;

    using StreamDescriptors = typename ContainerTypes::StreamDescriptors;
    static const Int Streams = ListSize<StreamDescriptors>::Value;

    using Position_         = core::StaticVector<typename ContainerTypes::CtrSizeT, Streams>;
    using Page              = typename ContainerTypes::Allocator::Page;

    using NodePageBase0     = TreeNodeBase<typename ContainerTypes::Metadata, Page>;
    using NodePageBase0G    = PageGuard<NodePageBase0, typename ContainerTypes::Allocator>;

    using CtrSizeT                  = typename ContainerTypes::CtrSizeT;

    using BranchStreamsStructList   = typename PackedBranchStructListBuilder<CtrSizeT, StreamDescriptors>::StructList;

    using LeafStreamsStructList     = typename PackedLeafStructListBuilder<CtrSizeT, StreamDescriptors>::StructList;
    using StreamsInputTypeList      = typename PackedLeafStructListBuilder<CtrSizeT, StreamDescriptors>::StreamInputList;
    using InputBufferStructList     = typename PackedLeafStructListBuilder<CtrSizeT, StreamDescriptors>::InputBufferList;

    using IteratorBranchNodeEntry = TypeListToTuple<
                Linearize<
                    typename IteratorBranchNodeEntryListBuilder<CtrSizeT, StreamDescriptors>::AccumTuple
                >
    >;

    using LeafRangeOffsetList = Linearize<
                    typename IteratorBranchNodeEntryListBuilder<CtrSizeT, StreamDescriptors>::RangeOffsetList
    >;

    using LeafRangeList = Linearize<
                    typename IteratorBranchNodeEntryListBuilder<CtrSizeT, StreamDescriptors>::IndexRangeList,
                    2
    >;


    using BranchNodeEntry_ = TypeListToTuple<typename BranchNodeEntryBuilder<Linearize<BranchStreamsStructList>>::Type>;

    struct NodeTypesBase: ContainerTypes {
        using NodeBase  = Page;
        using Name      = ContainerTypeName_;
        using Metadata  = typename ContainerTypes::Metadata;
        using ID        = typename MyType::ID;

        using BranchNodeEntry   = BranchNodeEntry_;
        using Position          = Position_;
        using CtrSizesT         = Position_;

        using LeafStreamsStructList     = typename MyType::LeafStreamsStructList;
        using BranchStreamsStructList   = typename MyType::BranchStreamsStructList;
        using IteratorBranchNodeEntry   = typename MyType::IteratorBranchNodeEntry;
        using StreamsInputTypeList      = typename MyType::StreamsInputTypeList;
        using InputBufferStructList     = typename MyType::InputBufferStructList;
    };

    struct BranchNodeTypes: NodeTypesBase {
    };

    struct LeafNodeTypes: NodeTypesBase {
    };

    struct DispatcherTypes
    {
        using NodeTypesList = typename ContainerTypes::NodeTypesList;

        using DefaultBranchNodeTypesList    = typename ContainerTypes::DefaultBranchNodeTypesList;
        using DefaultLeafNodeTypesList      = typename ContainerTypes::DefaultLeafNodeTypesList;

        using BranchNodeTypes   = typename MyType::BranchNodeTypes;
        using LeafNodeTypes     = typename MyType::LeafNodeTypes;

        using NodeBaseG         = NodePageBase0G;
    };

    typedef bt::BTreeDispatchers<DispatcherTypes>                              PageDispatchers;

    static const PackedSizeType BranchSizeType  = PackedListStructSizeType<Linearize<BranchStreamsStructList>>::Value;
    static const PackedSizeType LeafSizeType    = PackedListStructSizeType<Linearize<LeafStreamsStructList>>::Value;

    static const PackedSizeType TotalSizeType   = PackedSizeTypeList<BranchSizeType, LeafSizeType>::Value;



    using CtrListBranch = IfThenElse<
                            BranchSizeType == PackedSizeType::FIXED,
                            typename ContainerTypes::FixedBranchContainerPartsList,
                            typename ContainerTypes::VariableBranchContainerPartsList
    >;

    using CtrListLeaf = IfThenElse<
                        LeafSizeType == PackedSizeType::FIXED,
                        typename ContainerTypes::FixedLeafContainerPartsList,
                        typename ContainerTypes::VariableLeafContainerPartsList
    >;

    using CtrList = MergeLists<
            typename ContainerTypes::ContainerPartsList,
            MergeLists<CtrListLeaf, CtrListBranch, typename ContainerTypes::CommonContainerPartsList>
    >;


public:
    struct Types: ContainerTypes
    {
        typedef ContainerTypeName_                                              ContainerTypeName;
        typedef typename MyType::PageDispatchers                                Pages;

        typedef typename ContainerTypes::Allocator                              Allocator;
        typedef typename ContainerTypes::Metadata                               Metadata;

        typedef NodePageBase0                                                   NodeBase;
        typedef NodePageBase0G                                                  NodeBaseG;

        typedef typename MyType::CtrList                                        CtrList;
        typedef typename ContainerTypes::IteratorPartsList                      IterList;

        // FIXME Refactor BTree hierarchy
        // Use container types as base definitions
        typedef BTCtrTypes<Types>                                               CtrTypes;
        typedef BTIterTypes<Types>                                              IterTypes;

        static const Int Streams                                                = MyType::Streams;

        typedef BranchNodeEntry_                                                    BranchNodeEntry;

        using Position  = Position_;
        using CtrSizesT = Position_;

        typedef PageUpdateManager<CtrTypes>                                     PageUpdateMgr;

        using LeafStreamsStructList     = typename MyType::LeafStreamsStructList;

        using InputBufferStructList     = typename MyType::InputBufferStructList;

        using BranchStreamsStructList   = typename MyType::BranchStreamsStructList;

        using IteratorBranchNodeEntry   = typename MyType::IteratorBranchNodeEntry;
        using LeafRangeOffsetList       = typename MyType::LeafRangeOffsetList;
        using LeafRangeList             = typename MyType::LeafRangeList;

//        template <typename LeafPath>
//        using TargetType = typename PackedStructValueTypeH<LeafStreamsStructList, LeafPath>::Type;

        template <typename LeafPath>
        using TargetType = typename AccumType<
                BrachStructAccessorTool<
                    LeafStreamsStructList,
                    BranchStreamsStructList,
                    LeafPath
                >
        >::Type;

        template <typename LeafPath>
        using TargetType2 = typename AccumType<
                BrachStructAccessorTool<
                    LeafStreamsStructList,
                    BranchStreamsStructList,
                    LeafPath
                >
        >::Type;

        using StreamsInputTypeList = typename MyType::StreamsInputTypeList;

        template <Int Stream>
        using StreamInputTuple = TypeListToTuple<Select<Stream, StreamsInputTypeList>>;

        template <Int Stream>
        using InputTupleAdapter = StreamTupleHelper<StreamInputTuple<Stream>>;

        template <typename LeafPath>
        using AccumItemH = AccumItem<LeafStreamsStructList, LeafPath, IteratorBranchNodeEntry>;


        template <Int SubstreamIdx>
        using LeafPathT = typename v1::list_tree::BuildTreePath<LeafStreamsStructList, SubstreamIdx>::Type;

        template <Int SubstreamIdx>
        using BranchPathT = typename v1::list_tree::BuildTreePath<BranchStreamsStructList, SubstreamIdx>::Type;

        template <Int StreamIdx>
        using StreamInputBufferStructList = Select<StreamIdx, InputBufferStructList>;


        static const LeafDataLengthType LeafDataLength = LeafSizeType == PackedSizeType::FIXED ?
                                                        LeafDataLengthType::FIXED :
                                                        LeafDataLengthType::VARIABLE;


        using LeafType = typename Pages::LeafDispatcher::Head::Base;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;
};



}}
