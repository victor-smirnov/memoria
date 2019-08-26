
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
#include <memoria/v1/prototypes/bt/container/bt_c_tools_pl.hpp>
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
#include <memoria/v1/prototypes/bt/container/bt_c_remove.hpp>

#include <memoria/v1/prototypes/bt/bt_iterator.hpp>

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_insert_batch_common.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_insert_batch_fixed.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_insert_batch_variable.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_remove_batch.hpp>
#include <memoria/v1/prototypes/bt/container/bt_c_remove_tools.hpp>

#include <memoria/v1/profiles/common/common.hpp>


#include <type_traits>

namespace memoria {
namespace v1 {


template <typename Profile_, typename ContainerTypeSelector>
struct BTTypes {

    using Profile  = Profile_;

    using CtrSizeT = int64_t;

    using ContainerPartsList = TypeList<
            bt::ToolsName,
			bt::ToolsPLName,
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
            bt::IOReadName,
            bt::UpdateName,
            bt::WalkName
    >;


    using FixedBranchContainerPartsList = TypeList<
            bt::BranchFixedName,
            bt::InsertBatchFixedName
    >;

    using VariableBranchContainerPartsList = TypeList<
            bt::BranchVariableName,
            bt::InsertBatchVariableName
    >;

    using FixedLeafContainerPartsList = TypeList<
            bt::LeafFixedName
    >;

    using VariableLeafContainerPartsList = TypeList<
            bt::LeafVariableName
    >;
    
    using CommonContainerPartsList = TypeList<>;


    using IteratorPartsList = TypeList<
            bt::IteratorAPIName,
            bt::IteratorFindName,
            bt::IteratorSelectName,
            bt::IteratorRankName,
            bt::IteratorSkipName,
            bt::IteratorLeafName
    >;

    using IteratorInterface = EmptyType;

    using Allocator = ProfileAllocatorType<Profile_>;
    using ID        = ProfileBlockID<Profile_>;

    using Metadata  = BalancedTreeMetadata<ID>;

    using NodeTypesList = TypeList<
            bt::BranchNodeTypes<bt::BranchNode>,
            bt::LeafNodeTypes<bt::LeafNode>
    >;

    using DefaultBranchNodeTypesList = TypeList<
            bt::TreeNodeType<bt::BranchNode>
    >;

    using DefaultLeafNodeTypesList = TypeList<
            bt::TreeNodeType<bt::LeafNode>
    >;
    //FIXME DefaultNodeTypesList is not used anymore

    using StreamDescriptors = TypeList<>;

    template <typename Types_>
    using CtrBaseFactory = bt::BTreeCtrBase<Types_>;

    template <typename Types_>
    using IterBaseFactory = BTIteratorBase<Types_>;

    template <typename Iterator, typename Container>
    using IteratorCacheFactory = bt::BTreeIteratorPrefixCache<Iterator, Container>;


    template <typename Types, typename LeafPath>
    using FindGTForwardWalker          = bt::FindGTForwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindMaxGTWalker              = bt::FindMaxGTWalker<bt::WalkerTypes<Types, LeafPath>>;


    template <typename Types, typename LeafPath>
    using FindGTBackwardWalker         = bt::FindGTBackwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindGEForwardWalker          = bt::FindGEForwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindMaxGEWalker              = bt::FindMaxGEWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindGEBackwardWalker         = bt::FindGEBackwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SkipForwardWalker     = bt::SkipForwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SkipBackwardWalker    = bt::SkipBackwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SelectForwardWalker   = bt::SelectForwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SelectBackwardWalker  = bt::SelectBackwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using RankForwardWalker   = bt::RankForwardWalker<bt::RankWalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using RankBackwardWalker  = bt::RankBackwardWalker<bt::RankWalkerTypes<Types, LeafPath>>;

    template <typename Types>
    using NextLeafWalker      = bt::ForwardLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker      = bt::BackwardLeafWalker<Types>;

    template <typename Types, typename LeafPath>
    using NextLeafMutistreamWalker  = bt::SkipForwardWalker<bt::WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using PrevLeafMutistreamWalker  = bt::SkipBackwardWalker<bt::WalkerTypes<Types, LeafPath>>;
};




template <
        typename Profile,
        typename ContainerTypeName_
>
class CtrTF<Profile, BT, ContainerTypeName_> {

    using MyType = CtrTF<Profile, BT, ContainerTypeName_>;

public:

    using ContainerTypes = BTTypes<Profile, ContainerTypeName_>;
    using BlockID        = typename ContainerTypes::Allocator::BlockID;

    using StreamDescriptors         = typename ContainerTypes::StreamDescriptors;
    static const int32_t Streams    = ListSize<StreamDescriptors>;

    using Position_         = core::StaticVector<typename ContainerTypes::CtrSizeT, Streams>;
    using BlockType         = typename ContainerTypes::Allocator::BlockType;

    using TreeNodeBase      = bt::TreeNodeBase<typename ContainerTypes::Metadata, BlockType>;
    using TreeNodeBaseG     = BlockGuard<TreeNodeBase, typename ContainerTypes::Allocator>;

    using CtrSizeT                  = typename ContainerTypes::CtrSizeT;

    using BranchStreamsStructList   = typename bt::PackedBranchStructListBuilder<CtrSizeT, StreamDescriptors>::StructList;

    using LeafStreamsStructList     = typename bt::PackedLeafStructListBuilder<CtrSizeT, StreamDescriptors>::StructList;

    using IteratorBranchNodeEntry = bt::TypeListToTuple<
                Linearize<
                    typename bt::IteratorBranchNodeEntryListBuilder<CtrSizeT, StreamDescriptors>::AccumTuple
                >
    >;

    using LeafRangeOffsetList = Linearize<
                    typename bt::IteratorBranchNodeEntryListBuilder<CtrSizeT, StreamDescriptors>::RangeOffsetList
    >;

    using LeafRangeList = Linearize<
                    typename bt::IteratorBranchNodeEntryListBuilder<CtrSizeT, StreamDescriptors>::IndexRangeList,
                    2
    >;


    using BranchNodeEntry_ = AsTuple<typename bt::BranchNodeEntryBuilder<Linearize<BranchStreamsStructList>>::Type>;

    struct NodeTypesBase: ContainerTypes
    {
        using NodeBase  = TreeNodeBase;
        using Name      = ContainerTypeName_;
        using Metadata  = typename ContainerTypes::Metadata;
        using BlockID   = typename MyType::BlockID;

        using BranchNodeEntry = BranchNodeEntry_;
        using Position        = Position_;
        using CtrSizesT       = Position_;

        using LeafStreamsStructList     = typename MyType::LeafStreamsStructList;
        using BranchStreamsStructList   = typename MyType::BranchStreamsStructList;
        using IteratorBranchNodeEntry   = typename MyType::IteratorBranchNodeEntry;
    };

    struct BranchNodeTypes: NodeTypesBase {};
    struct LeafNodeTypes: NodeTypesBase {};

    struct DispatcherTypes
    {
        using NodeTypesList = typename ContainerTypes::NodeTypesList;

        using DefaultBranchNodeTypesList    = typename ContainerTypes::DefaultBranchNodeTypesList;
        using DefaultLeafNodeTypesList      = typename ContainerTypes::DefaultLeafNodeTypesList;

        using BranchNodeTypes   = typename MyType::BranchNodeTypes;
        using LeafNodeTypes     = typename MyType::LeafNodeTypes;

        using NodeBaseG         = TreeNodeBaseG;
    };

    using BlockDispatchers = bt::BTreeDispatchers<DispatcherTypes>;

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

    using CtrExtensionsList  = typename bt::ContainerExtensionsTF<Profile, ContainerTypeName_>::Type;
    using IterExtensionsList = typename bt::IteratorExtensionsTF<Profile, ContainerTypeName_>::Type;

    using CtrList = MergeLists<
            typename ContainerTypes::ContainerPartsList,
            MergeLists<CtrExtensionsList, CtrListLeaf, CtrListBranch, typename ContainerTypes::CommonContainerPartsList>
    >;

    using IterList = MergeLists<IterExtensionsList, typename ContainerTypes::IteratorPartsList>;


public:
    struct Types: ContainerTypes
    {
        using ContainerTypeName = ContainerTypeName_;
        using Blocks = typename MyType::BlockDispatchers;

        using Allocator = typename ContainerTypes::Allocator;
        using Metadata  = typename ContainerTypes::Metadata;

        using NodeBaseG = TreeNodeBaseG;

        using CtrList  = typename MyType::CtrList;
        using IterList = typename MyType::IterList;

        // FIXME Refactor BTree hierarchy
        // Use container types as base definitions
        using CtrTypes  = BTCtrTypes<Types>;
        using IterTypes = BTIterTypes<Types>;

        static constexpr int32_t Streams = MyType::Streams;

        using BranchNodeEntry = BranchNodeEntry_;

        using Position  = Position_;
        using CtrSizesT = Position_;

        using BlockUpdateMgr = bt::BlockUpdateManager<CtrTypes>;

        using LeafStreamsStructList     = typename MyType::LeafStreamsStructList;

        using BranchStreamsStructList   = typename MyType::BranchStreamsStructList;

        using IteratorBranchNodeEntry   = typename MyType::IteratorBranchNodeEntry;
        using LeafRangeOffsetList       = typename MyType::LeafRangeOffsetList;
        using LeafRangeList             = typename MyType::LeafRangeList;


        template <typename LeafPath>
        using TargetType = typename AccumType<
                bt::BrachStructAccessorTool<
                    LeafStreamsStructList,
                    BranchStreamsStructList,
                    LeafPath
                >
        >::Type;


        template <typename LeafPath>
        using AccumItemH = bt::AccumItem<LeafStreamsStructList, LeafPath, IteratorBranchNodeEntry>;


        template <int32_t SubstreamIdx>
        using LeafPathT   = typename list_tree::BuildTreePath<LeafStreamsStructList, SubstreamIdx>::Type;

        template <int32_t SubstreamIdx>
        using BranchPathT = typename list_tree::BuildTreePath<BranchStreamsStructList, SubstreamIdx>::Type;

        template <typename CtrT, typename SubstreamPath>
        using LeafPackedStruct = typename Blocks::template LeafDispatcher<CtrT>::Head::template PackedStruct<SubstreamPath>;


        using BranchNode = typename ListHead<typename Blocks::BranchDTypes::List>::Type;
        using LeafNode   = typename ListHead<typename Blocks::LeafDTypes::List>::Type;

        static const LeafDataLengthType LeafDataLength = LeafSizeType == PackedSizeType::FIXED ?
                                                        LeafDataLengthType::FIXED :
                                                        LeafDataLengthType::VARIABLE;

        using LeafType   = typename LeafNode::Base;
        using BranchType = typename BranchNode::Base;
    };

    using CtrTypes = typename Types::CtrTypes;
    using Type     = Ctr<CtrTypes>;
};



}}
