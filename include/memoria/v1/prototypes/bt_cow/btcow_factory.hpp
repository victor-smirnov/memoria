
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

#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/tools/vector_tuple.hpp>

#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>

#include <memoria/v1/prototypes/bt_cow/btcow_names.hpp>
#include <memoria/v1/prototypes/bt_cow/tools/btcow_tools.hpp>
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
#include <memoria/v1/prototypes/bt/container/bt_c_ioread.hpp>
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

#include <memoria/v1/prototypes/bt_cow/btcow_names.hpp>

#include <memoria/v1/prototypes/bt_cow/btcow_iterator.hpp>

#include <memoria/v1/prototypes/bt_cow/container/btcow_c_allocator.hpp>
#include <memoria/v1/prototypes/bt_cow/container/btcow_c_tools.hpp>
#include <memoria/v1/prototypes/bt_cow/container/btcow_c_find.hpp>
#include <memoria/v1/prototypes/bt_cow/container/btcow_c_branch_fixed.hpp>
#include <memoria/v1/prototypes/bt_cow/container/btcow_c_branch_variable.hpp>
#include <memoria/v1/prototypes/bt_cow/container/btcow_c_leaf_fixed.hpp>
#include <memoria/v1/prototypes/bt_cow/container/btcow_c_leaf_variable.hpp>
#include <memoria/v1/prototypes/bt_cow/container/btcow_c_checks.hpp>
#include <memoria/v1/prototypes/bt_cow/container/btcow_c_walk.hpp>

#include <memoria/v1/prototypes/bt_cow/tools/btcow_tools.hpp>
#include <memoria/v1/prototypes/bt_cow/nodes/btcow_node_base.hpp>

namespace memoria {
namespace v1 {

using bt::WalkerTypes;

template <typename Profile_, typename ContainerTypeSelector>
struct BTCowTypes {

    using Profile  = Profile_;

    using CtrSizeT = int64_t;

    using ContainerPartsList = TypeList<
    		bt::ToolsName,
            btcow::AllocatorName,
            btcow::ToolsName,
            btcow::ChecksName,
            bt::BranchCommonName,
            bt::LeafCommonName,
            bt::InsertName,
            bt::RemoveToolsName,
            bt::RemoveName,
            btcow::FindName,
            bt::ReadName,
            bt::IOReadName,
            bt::UpdateName,
            btcow::WalkName
    >;


    using FixedBranchContainerPartsList 	= TypeList<
            btcow::BranchFixedName
    >;

    using VariableBranchContainerPartsList 	= TypeList<
            btcow::BranchVariableName
    >;

    using FixedLeafContainerPartsList 		= TypeList<
            btcow::LeafFixedName
    >;

    using VariableLeafContainerPartsList 	= TypeList<
            btcow::LeafVariableName
    >;
    
    using CommonContainerPartsList 		 	= TypeList<>;


    using IteratorPartsList = TypeList<
            bt::IteratorAPIName,
            bt::IteratorFindName,
            bt::IteratorSelectName,
            bt::IteratorRankName,
            bt::IteratorSkipName,
            bt::IteratorLeafName
    >;

    using ContainerInterface = EmptyType;
    using IteratorInterface  = EmptyType;
    using IteratorData 		 = EmptyType;


    using Allocator = typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator;
    using ID        = typename Allocator::ID;

    using Metadata  = BalancedTreeMetadata<ID>;

    using NodeTypesList = TypeList<
            BranchNodeTypes<BranchNode>,
            LeafNodeTypes<LeafNode>
    >;

    using DefaultBranchNodeTypesList = TypeList<
            TreeNodeType<BranchNode>
    >;

    using DefaultLeafNodeTypesList = TypeList<
            TreeNodeType<LeafNode>
    >;


    using StreamDescriptors = TypeList<>;

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
        typedef btcow::BTreeIteratorPrefixCache<Iterator, Container>   Type;
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
class CtrTF<Profile, BTCow, ContainerTypeName_> {

    using MyType = CtrTF<Profile, BTCow, ContainerTypeName_>;

public:

    using ContainerTypes = BTCowTypes<Profile, ContainerTypeName_>;
    using ID             = typename ContainerTypes::Allocator::Page::ID;

    using StreamDescriptors = typename ContainerTypes::StreamDescriptors;
    static const int32_t Streams = ListSize<StreamDescriptors>::Value;

    using Position_         = core::StaticVector<typename ContainerTypes::CtrSizeT, Streams>;
    using Page              = typename ContainerTypes::Allocator::Page;

    using NodePageBase0     = btcow::TreeNodeBase<typename ContainerTypes::Metadata, Page>;
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


    using BranchNodeEntry_ = AsTuple<typename BranchNodeEntryBuilder<Linearize<BranchStreamsStructList>>::Type>;

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

        template <typename Metadata, typename NodeBase>
        using TreeNodeBaseTF = btcow::TreeNodeBase<Metadata, NodeBase>;
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

    using PageDispatchers 	= bt::BTreeDispatchers<DispatcherTypes>;
    using TreePath 			= core::StaticArray<NodePageBase0G, 8>;

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

    using CtrExtensionsList  = typename ContainerExtensionsTF<Profile, ContainerTypeName_>::Type;
    using IterExtensionsList = typename IteratorExtensionsTF<Profile, ContainerTypeName_>::Type;

    using CtrList = MergeLists<
            typename ContainerTypes::ContainerPartsList,
            MergeLists<CtrExtensionsList, CtrListLeaf, CtrListBranch, typename ContainerTypes::CommonContainerPartsList>
    >;

    using IterList = MergeLists<IterExtensionsList, typename ContainerTypes::IteratorPartsList>;


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
        typedef typename MyType::IterList                                       IterList;

        // FIXME Refactor BTree hierarchy
        // Use container types as base definitions
        using CtrTypes  = BTCowCtrTypes<Types>;
        using IterTypes = BTCowIterTypes<Types>;

        using TreePath = core::StaticArray<NodeBaseG, 8>;

        static constexpr int32_t Streams = MyType::Streams;

        using BranchNodeEntry = BranchNodeEntry_;

        using Position  = Position_;
        using CtrSizesT = Position_;

        using PageUpdateMgr = PageUpdateManager<CtrTypes>;

        using LeafStreamsStructList     = typename MyType::LeafStreamsStructList;

        using InputBufferStructList     = typename MyType::InputBufferStructList;

        using BranchStreamsStructList   = typename MyType::BranchStreamsStructList;

        using IteratorBranchNodeEntry   = typename MyType::IteratorBranchNodeEntry;
        using LeafRangeOffsetList       = typename MyType::LeafRangeOffsetList;
        using LeafRangeList             = typename MyType::LeafRangeList;


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

        template <int32_t Stream>
        using StreamInputTuple  = TypeListToTuple<Select<Stream, StreamsInputTypeList>>;

        template <int32_t Stream>
        using InputTupleAdapter = StreamTupleHelper<StreamInputTuple<Stream>>;

        template <typename LeafPath>
        using AccumItemH = AccumItem<LeafStreamsStructList, LeafPath, IteratorBranchNodeEntry>;


        template <int32_t SubstreamIdx>
        using LeafPathT   = typename list_tree::BuildTreePath<LeafStreamsStructList, SubstreamIdx>::Type;

        template <int32_t SubstreamIdx>
        using BranchPathT = typename list_tree::BuildTreePath<BranchStreamsStructList, SubstreamIdx>::Type;

        template <int32_t StreamIdx>
        using StreamInputBufferStructList = Select<StreamIdx, InputBufferStructList>;

        template <typename SubstreamPath>
        using LeafPackedStruct = typename Pages::LeafDispatcher::Head::template PackedStruct<SubstreamPath>;


        using LeafNode = typename Pages::LeafDispatcher::Head;

        static const LeafDataLengthType LeafDataLength = LeafSizeType == PackedSizeType::FIXED ?
                                                        LeafDataLengthType::FIXED :
                                                        LeafDataLengthType::VARIABLE;


        using LeafType = typename Pages::LeafDispatcher::Head::Base;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type 		= Ctr<CtrTypes>;
};



}}
