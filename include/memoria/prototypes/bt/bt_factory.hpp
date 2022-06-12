
// Copyright 2011-2022 Victor Smirnov
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


#include <memoria/prototypes/bt/tools/bt_tools_vector_tuple.hpp>

#include <memoria/core/packed/packed.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/prototypes/bt/shuttles/bt_find_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_skip_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_select_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_rank_shuttle.hpp>

#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/prototypes/bt/nodes/node_dispatcher.hpp>
#include <memoria/prototypes/bt/nodes/tree_metadata.hpp>
#include <memoria/prototypes/bt/nodes/node_list_builder.hpp>

#include <memoria/prototypes/bt/container/bt_c_base.hpp>
#include <memoria/prototypes/bt/container/bt_c_tools.hpp>
#include <memoria/prototypes/bt/container/bt_c_tools_pl.hpp>
#include <memoria/prototypes/bt/container/bt_c_checks.hpp>
#include <memoria/prototypes/bt/container/bt_c_insert.hpp>
#include <memoria/prototypes/bt/container/bt_c_read.hpp>
#include <memoria/prototypes/bt/container/bt_c_update.hpp>
#include <memoria/prototypes/bt/container/bt_c_branch_common.hpp>
#include <memoria/prototypes/bt/container/bt_c_branch_variable.hpp>
#include <memoria/prototypes/bt/container/bt_c_branch_fixed.hpp>
#include <memoria/prototypes/bt/container/bt_c_leaf_common.hpp>
#include <memoria/prototypes/bt/container/bt_c_leaf_variable.hpp>
#include <memoria/prototypes/bt/container/bt_c_leaf_fixed.hpp>
#include <memoria/prototypes/bt/container/bt_c_find.hpp>
#include <memoria/prototypes/bt/container/bt_c_walk.hpp>
#include <memoria/prototypes/bt/container/bt_c_block.hpp>
#include <memoria/prototypes/bt/container/bt_c_cow.hpp>
#include <memoria/prototypes/bt/container/bt_c_no_cow.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/prototypes/bt/bt_iterator.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/prototypes/bt/container/bt_c_insert_batch_common.hpp>
#include <memoria/prototypes/bt/container/bt_c_insert_batch_fixed.hpp>
#include <memoria/prototypes/bt/container/bt_c_insert_batch_variable.hpp>
#include <memoria/prototypes/bt/container/bt_c_remove_batch.hpp>
#include <memoria/prototypes/bt/container/bt_c_node_common.hpp>

#include <memoria/profiles/common/common.hpp>


#include <type_traits>

namespace memoria {


template <typename Profile_, typename ContainerTypeSelector>
struct BTTypes {

    using Profile  = Profile_;

    using CtrSizeT = ProfileCtrSizeT<Profile>;

    using ContainerPartsList = TypeList<
            bt::ToolsName,
			bt::ToolsPLName,
            bt::ChecksName,
            bt::BranchCommonName,
            bt::InsertBatchCommonName,
            bt::LeafCommonName,
            bt::InsertName,
            bt::NodeCommonName,
            bt::RemoveBatchName,
            bt::FindName,
            bt::UpdateName,
            bt::WalkName,
            bt::BlockName,
            IfThenElse<ProfileTraits<Profile>::IsCoW, bt::CoWOpsName, bt::NoCoWOpsName>
    >;

    using RWContainerPartsList = TypeList<
        bt::ReadName
    >;

    using FixedBranchContainerPartsList = TypeList<
            bt::BranchFixedName,
            bt::InsertBatchFixedName
    >;

    using RWFixedBranchContainerPartsList = TypeList<>;

    using VariableBranchContainerPartsList = TypeList<
            bt::BranchVariableName,
            bt::InsertBatchVariableName
    >;
    using RWVariableBranchContainerPartsList = TypeList<>;


    using FixedLeafContainerPartsList = TypeList<
            bt::LeafFixedName
    >;

    using RWFixedLeafContainerPartsList = TypeList<>;

    using VariableLeafContainerPartsList = TypeList<
            bt::LeafVariableName
    >;

    using RWVariableLeafContainerPartsList = TypeList<>;
    
    using CommonContainerPartsList = TypeList<>;
    using RWCommonContainerPartsList = TypeList<>;

    using BlockIteratorStatePartsList = TypeList<>;

    using BlockIteratorStateInterface = EmptyType;

    using Allocator = ProfileStoreType<Profile_>;
    using ID        = ProfileBlockID<Profile_>;

    using Metadata  = BalancedTreeMetadata<Profile_>;

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

    using StreamDescriptors = TypeList<>;

    template <typename Types_>
    using CtrBaseFactory = bt::BTreeCtrBase<Types_>;

    template <typename Types_>
    using RWCtrBaseFactory = RWCtrBase<Types_>;

    template <typename Types_>
    using BlockIterStateBaseFactory = BTBlockIteratorStateBase<Types_>;
};




template <
        typename Profile,
        typename ContainerTypeName_
>
class CtrTF<Profile, BT, ContainerTypeName_> {

    using MyType = CtrTF<Profile, BT, ContainerTypeName_>;

public:

    using ContainerTypes = BTTypes<Profile, ContainerTypeName_>;
    using BlockID        = ProfileBlockID<Profile>;

    using StreamDescriptors         = typename ContainerTypes::StreamDescriptors;
    static const size_t Streams     = ListSize<StreamDescriptors>;

    using CtrSizeT                  = typename ContainerTypes::CtrSizeT;

    using Position_         = core::StaticVector<CtrSizeT, Streams>;
    using BlockType         = ProfileBlockType<Profile>;

    using TreeNodeBase         = bt::TreeNodeBase<typename ContainerTypes::Metadata, BlockType>;
    using TreeNodePtrT         = typename ProfileTraits<Profile>::template SharedBlockPtrTF<TreeNodeBase>;
    using TreeNodeConstPtrT    = typename ProfileTraits<Profile>::template SharedBlockPtrTF<const TreeNodeBase>;

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

        using TreeNodePtr       = TreeNodePtrT;
        using TreeNodeConstPtr  = TreeNodeConstPtrT;
    };

    using BlockDispatchers = bt::BTreeDispatchers<DispatcherTypes>;

    static const PackedDataTypeSize BranchSizeType  = PackedListStructSizeType<Linearize<BranchStreamsStructList>>::Value;
    static const PackedDataTypeSize LeafSizeType    = PackedListStructSizeType<Linearize<LeafStreamsStructList>>::Value;

    static const PackedDataTypeSize TotalSizeType   = PackedSizeTypeList<BranchSizeType, LeafSizeType>::Value;

    using CtrListBranch = IfThenElse<
                        BranchSizeType == PackedDataTypeSize::FIXED,
                        typename ContainerTypes::FixedBranchContainerPartsList,
                        typename ContainerTypes::VariableBranchContainerPartsList
    >;

    using RWCtrListBranch = IfThenElse<
                        BranchSizeType == PackedDataTypeSize::FIXED,
                        typename ContainerTypes::RWFixedBranchContainerPartsList,
                        typename ContainerTypes::RWVariableBranchContainerPartsList
    >;

    using CtrListLeaf = IfThenElse<
                        LeafSizeType == PackedDataTypeSize::FIXED,
                        typename ContainerTypes::FixedLeafContainerPartsList,
                        typename ContainerTypes::VariableLeafContainerPartsList
    >;

    using RWCtrListLeaf = IfThenElse<
                        LeafSizeType == PackedDataTypeSize::FIXED,
                        typename ContainerTypes::RWFixedLeafContainerPartsList,
                        typename ContainerTypes::RWVariableLeafContainerPartsList
    >;

    using CtrExtensionsList = typename bt::ContainerExtensionsTF<Profile, ContainerTypeName_>::Type;
    using RWCtrExtensionsList = typename bt::RWContainerExtensionsTF<Profile, ContainerTypeName_>::Type;

    using BlockIterStateExtensionsList = typename bt::BlockIteratorStateExtensionsTF<Profile, ContainerTypeName_>::Type;

    using CtrList = MergeLists<
            typename ContainerTypes::ContainerPartsList,
            MergeLists<CtrExtensionsList, CtrListLeaf, CtrListBranch, typename ContainerTypes::CommonContainerPartsList>
    >;

    using RWCtrList = MergeLists<
            typename ContainerTypes::RWContainerPartsList,
            MergeLists<RWCtrExtensionsList, RWCtrListLeaf, RWCtrListBranch, typename ContainerTypes::RWCommonContainerPartsList>
    >;

    using BlockIterStateList = MergeLists<BlockIterStateExtensionsList, typename ContainerTypes::BlockIteratorStatePartsList>;

public:
    struct Types: ContainerTypes
    {
        using ContainerTypeName = ContainerTypeName_;
        using Blocks = typename MyType::BlockDispatchers;

        using Allocator = typename ContainerTypes::Allocator;
        using Metadata  = typename ContainerTypes::Metadata;

        using TreeNodePtr = TreeNodePtrT;
        using TreeNodeConstPtr = TreeNodeConstPtrT;

        using CtrList  = typename MyType::CtrList;
        using RWCtrList  = typename MyType::RWCtrList;
        using BlockIterStateList = typename MyType::BlockIterStateList;

        // FIXME Refactor BTree hierarchy
        // Use container types as base definitions
        using CtrTypes  = CtrTypesT<Types>;
        using RWCtrTypes  = RWCtrTypesT<Types>;

        using BlockIterStateTypes = BTBlockIterStateTypes<Types>;

        static constexpr size_t Streams = MyType::Streams;

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
        using TargetType = AccumType<
                bt::BranchStructAccessorTool<
                    LeafStreamsStructList,
                    BranchStreamsStructList,
                    LeafPath
                >
        >;

        template <typename LeafPath>
        static constexpr DTOrdering KeyOrderingType = KeyOrdering<
                bt::BranchStructAccessorTool<
                    LeafStreamsStructList,
                    BranchStreamsStructList,
                    LeafPath
                >
        >;


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

        static const LeafDataLengthType LeafDataLength = LeafSizeType == PackedDataTypeSize::FIXED ?
                                                        LeafDataLengthType::FIXED :
                                                        LeafDataLengthType::VARIABLE;

        using LeafType   = typename LeafNode::Base;
        using BranchType = typename BranchNode::Base;
    };

    using CtrTypes = typename Types::CtrTypes;
    using RWCtrTypes = typename Types::RWCtrTypes;

    using Type     = Ctr<CtrTypes>;
    using RWType   = Ctr<RWCtrTypes>;
};



}
