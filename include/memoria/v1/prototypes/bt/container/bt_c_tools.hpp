
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

#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/strings/string.hpp>

#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <iostream>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ToolsName)
public:
    using Types = typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Allocator::BlockG                                     BlockG;

    using typename Base::BlockID;
    using typename Base::BlockType;

    using Profile = typename Types::Profile;

    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

public:
    NodeBaseG ctr_get_root_node() const
    {
        auto& self = this->self();
        return self.store().getBlock(self.root()).get_or_terminate();
    }

    NodeBaseG ctr_get_root_node_for_update() const
    {
        auto& self = this->self();
        return self.store().getBlockForUpdate(self.root()).get_or_terminate();
    }


    MEMORIA_V1_DECLARE_NODE_FN(DumpBlockSizesFn, ctr_dump_block_sizes);
    void ctr_dump_block_sizes(const NodeBaseG& node) const
    {
        self().leaf_dispatcher().dispatch(node, DumpBlockSizesFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(MaxFn, max);
    BranchNodeEntry ctr_get_node_max_keys(const NodeBaseG& node) const
    {
        BranchNodeEntry entry;
        self().node_dispatcher().dispatch(node, MaxFn(), entry);
        return entry;
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetSizesFn, sizes, Position);
    Position ctr_get_node_sizes(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, GetSizesFn());
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetSizeFn, size, int32_t);
    int32_t ctr_get_node_size(const NodeBaseG& node, int32_t stream) const
    {
        return self().node_dispatcher().dispatch(node, GetSizeFn(), stream);
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(CheckCapacitiesFn, checkCapacities, bool);
    bool ctr_check_node_capacities(const NodeBaseG& node, const Position& sizes) const
    {
        return self().node_dispatcher().dispatch(node, CheckCapacitiesFn(), sizes);
    }

    MEMORIA_V1_DECLARE_NODE_FN(GenerateDataEventsFn, generateDataEvents);
    void ctr_dump_node(const NodeBaseG& block, std::ostream& out = std::cout) const
    {
        const auto& self = this->self();

        if (block)
        {
            TextBlockDumper dumper(out);

            self.node_dispatcher().dispatch(block, GenerateDataEventsFn(), &dumper);

            out<<std::endl;
            out<<std::endl;
        }
        else {
            out << "NULL" << std::endl;
        }
    }

    void ctr_dump_path(NodeBaseG node, std::ostream& out = std::cout, int32_t depth = 100) const
    {
        auto& self = this->self();

        out << "Path:" << std::endl;

        self.ctr_dump_node(node, out);

        while (!node->is_root() && node->level() < depth)
        {
            node = self.ctr_get_node_parent(node);
            self.ctr_dump_node(node, out);
        }
    }


protected:

    bool ctr_is_the_same_sode(const NodeBaseG& node1, const NodeBaseG& node2) const
    {
        return node1->id() == node2->id();
    }




    template <typename Node>
    NodeBaseG ctr_get_child_fn(Node&& node, int32_t idx) const
    {
        auto& self = this->self();
        return self.store().getBlock(node.value(idx)).get_or_terminate();
    }

    template <typename Node>
    NodeBaseG getLastChildFn(Node&& node, int32_t idx) const
    {
        auto& self = this->self();
        return self.store().getBlock(node.value(node->size() - 1)).get_or_terminate();
    }


    template <typename Node>
    NodeBaseG getChildForUpdateFn(Node&& node, int32_t idx) const
    {
        auto& self = this->self();
        return self.store().getBlockForUpdate(node.value(idx)).get_or_terminate();
    }


    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetChildFn, ctr_get_child_fn, NodeBaseG);
    NodeBaseG ctr_get_node_child(const NodeBaseG& node, int32_t idx) const
    {
        NodeBaseG result = self().branch_dispatcher().dispatch(node, GetChildFn(self()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            MMA1_THROW(NullPointerException()) << WhatCInfo("Child must not be NULL");
        }
    }

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetLastChildFn, getLastChildFn, NodeBaseG);
    NodeBaseG ctr_get_node_last_child(const NodeBaseG& node, int32_t idx) const
    {
        NodeBaseG result = self().branch_dispatcher().dispatch(node, GetLastChildFn(self()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            MMA1_THROW(NullPointerException()) << WhatCInfo("Child must not be NULL");
        }
    }



    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetChildForUpdateFn, getChildForUpdateFn, NodeBaseG);
    NodeBaseG ctr_get_child_for_update(const NodeBaseG& node, int32_t idx) const
    {
        NodeBaseG result = self().branch_dispatcher().dispatch(node, GetChildForUpdateFn(self()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            MMA1_THROW(NullPointerException()) << WhatCInfo("Child must not be NULL");
        }
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(NodeStreamSizesFn, size_sums, Position);
    Position ctr_node_stream_sizes(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, NodeStreamSizesFn());
    }



    MEMORIA_V1_DECLARE_NODE_FN(SumsFn, sums);
    void ctr_sums(const NodeBaseG& node, BranchNodeEntry& sums) const
    {
        self().node_dispatcher().dispatch(node, SumsFn(), sums);
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SumsRtnFn, sums, BranchNodeEntry);
    BranchNodeEntry ctr_sums(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, SumsRtnFn());
    }

    void ctr_sums(const NodeBaseG& node, int32_t start, int32_t end, BranchNodeEntry& sums) const
    {
        self().node_dispatcher().dispatch(node, SumsFn(), start, end, sums);
    }

    BranchNodeEntry ctr_sums(const NodeBaseG& node, int32_t start, int32_t end) const
    {
        BranchNodeEntry sums;
        self().node_dispatcher().dispatch(node, SumsFn(), start, end, sums);
        return sums;
    }

    void ctr_sums(const NodeBaseG& node, const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        self().node_dispatcher().dispatch(node, SumsFn(), start, end, sums);
    }

    BranchNodeEntry ctr_sums(const NodeBaseG& node, const Position& start, const Position& end) const
    {
        BranchNodeEntry sums;
        self().node_dispatcher().dispatch(node, SumsFn(), start, end, sums);
        return sums;
    }



    template <typename Path>
    struct LeafSumsFn {
        template <typename Node, typename... Args>
        auto treeNode(Node&& node, Args&&... args) {
            return node->template ctr_leaf_sums<Path>(std::forward<Args>(args)...);
        }
    };


    struct LeafSizesFn {
        template <typename Node, typename... Args>
        auto treeNode(Node&& node, Args&&... args) {
            return node.sizes();
        }
    };

    template <typename Path, typename... Args>
    auto ctr_leaf_sums(const NodeBaseG& node, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(node, LeafSumsFn<Path>(), std::forward<Args>(args)...);
    }


    auto ctr_leaf_sizes(const NodeBaseG& node) const
    {
        return self().leaf_dispatcher().dispatch(node, LeafSizesFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetINodeDataFn, value, BlockID);
    BlockID ctr_get_child_id(const NodeBaseG& node, int32_t idx) const
    {
        return self().branch_dispatcher().dispatch(node, GetINodeDataFn(), idx);
    }


public:

    NodeBaseG ctr_get_next_node(NodeBaseG& node) const;
    NodeBaseG ctr_get_prev_node(NodeBaseG& node) const;


    MEMORIA_V1_DECLARE_NODE_FN(LayoutNodeFn, layout);
    void ctr_layout_branch_node(NodeBaseG& node, uint64_t active_streams) const
    {
        self().branch_dispatcher().dispatch(node, LayoutNodeFn(), active_streams);
    }

    void ctr_layout_leaf_node(NodeBaseG& node, const Position& sizes) const
    {
        self().leaf_dispatcher().dispatch(node, LayoutNodeFn(), sizes);
    }


protected:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetActiveStreamsFn, active_streams, uint64_t);
    uint64_t ctr_get_active_streams(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, GetActiveStreamsFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(IsNodeEmpty, is_empty, bool);
    bool ctr_is_node_empty(const NodeBaseG& node)
    {
        return self().node_dispatcher().dispatch(node, IsNodeEmpty());
    }


    MEMORIA_V1_DECLARE_NODE_FN(ForAllIDsFn, forAllValues);
    void ctr_for_all_ids(const NodeBaseG& node, int32_t start, int32_t end, std::function<void (const BlockID&, int32_t)> fn) const
    {
        self().branch_dispatcher().dispatch(node, ForAllIDsFn(), start, end, fn);
    }

    void ctr_for_all_ids(const NodeBaseG& node, int32_t start, std::function<void (const BlockID&, int32_t)> fn) const
    {
        self().branch_dispatcher().dispatch(node, ForAllIDsFn(), start, fn);
    }

    void ctr_for_all_ids(const NodeBaseG& node, std::function<void (const BlockID&, int32_t)> fn) const
    {
        self().branch_dispatcher().dispatch(node, ForAllIDsFn(), fn);
    }


    struct SetChildIDFn {
        template <typename CtrT, typename T>
        void treeNode(BranchNodeSO<CtrT, T>& node, int child_idx, const BlockID& child_id) const
        {
            node.value(child_idx) = child_id;
        }
    };

    void ctr_set_child_id(NodeBaseG& node, int32_t child_idx, const BlockID& child_id) const
    {
        self().ctr_update_block_guard(node);
        self().branch_dispatcher().dispatch(node, SetChildIDFn(), child_idx, child_id);
    }



    struct GetBranchNodeChildernCount {
        template <typename CtrT, typename T, typename... Args>
        int32_t treeNode(BranchNodeSO<CtrT, T>& node, Args&&... args) const
        {
            return node->size();
        }
    };

    template <int32_t Stream>
    struct GetLeafNodeStreamSize {
        template <typename CtrT, typename T, typename... Args>
        int32_t treeNode(const LeafNodeSO<CtrT, T>& node, Args&&... args) const
        {
            return node.template streamSize<Stream>();
        }
    };

    struct GetLeafNodeStreamSizes {
        template <typename CtrT, typename T, typename... Args>
        Position treeNode(const LeafNodeSO<CtrT, T>& node, Args&&... args) const
        {
            return node.sizes();
        }
    };

    struct GetLeafNodeStreamSizesStatic {
        template <typename CtrT, typename T, typename... Args>
        Position treeNode(const LeafNodeSO<CtrT, T>& node, Args&&... args) const
        {
            return bt::LeafNode<T>::sizes(std::forward<Args>(args)...);
        }
    };


    int32_t ctr_get_node_children_count(const NodeBaseG& node) const
    {
        return self().branch_dispatcher().dispatch(node, GetBranchNodeChildernCount());
    }



    int32_t ctr_get_branch_node_size(const NodeBaseG& node) const
    {
        return self().branch_dispatcher().dispatch(node, GetSizeFn());
    }

public:

    template <int32_t StreamIdx>
    int32_t ctr_get_leaf_stream_size(const NodeBaseG& node) const
    {
        return self().leaf_dispatcher().dispatch(node, GetLeafNodeStreamSize<StreamIdx>());
    }

    Position ctr_get_leaf_stream_sizes(const NodeBaseG& node) const
    {
        return self().leaf_dispatcher().dispatch(node, GetLeafNodeStreamSizes());
    }


    Position get_get_stream_sizes(const BranchNodeEntry& sums) const
    {
        return self().leaf_dispatcher().template dispatch<bt::LeafNode>(true, GetLeafNodeStreamSizesStatic(), sums);
    }

    struct CreateIOVectorViewFn
    {
        template <typename T, typename... Args>
        std::unique_ptr<io::IOVector> treeNode(T&& node) const
        {
            return node->create_iovector_view();
        }
    };

    std::unique_ptr<io::IOVector> create_iovector_view(NodeBaseG& node)
    {
        return self().leaf_dispatcher().dispatch(node, CreateIOVectorViewFn());
    }


    struct ConfigureIOVectorViewFn
    {
        template <typename T, typename... Args>
        void treeNode(T&& node, io::IOVector& io_vector) const
        {
            return node->configure_iovector_view(io_vector);
        }
    };

    void configure_iovector_view(NodeBaseG& node, io::IOVector& io_vector)
    {
        return self().leaf_dispatcher().dispatch(node, ConfigureIOVectorViewFn(), io_vector);
    }

protected:

    template <typename SubstreamPath>
    struct GetPackedStructFn {
        template <typename CtrT, typename T>
        auto treeNode(LeafNodeSO<CtrT, T>& node) const
        {
            return node.template substream<SubstreamPath>();
        }

        template <typename CtrT, typename T>
        auto treeNode(const LeafNodeSO<CtrT, T>& node) const
        {
            return node.template substream<SubstreamPath>();
        }
    };



    template <typename SubstreamPath>
    const auto* ctr_get_packed_struct(const NodeBaseG& leaf) const
    {
        return self().leaf_dispatcher().dispatch(leaf, GetPackedStructFn<SubstreamPath>());
    }

    template <typename SubstreamPath>
    auto* ctr_get_packed_struct(NodeBaseG& leaf)
    {
        return self().leaf_dispatcher().dispatch(leaf, GetPackedStructFn<SubstreamPath>());
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS






M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::ctr_get_next_node(NodeBaseG& node) const
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent(node);

        int32_t size = self.ctr_get_node_size(parent, 0);

        int32_t parent_idx = node->parent_idx();

        if (parent_idx < size - 1)
        {
            return self.ctr_get_node_child(parent, parent_idx + 1);
        }
        else {
            NodeBaseG target_parent = ctr_get_next_node(parent);

            if (target_parent.isSet())
            {
                return self.ctr_get_node_child(target_parent, 0);
            }
            else {
                return target_parent;
            }
        }
    }
    else {
        return NodeBaseG();
    }
}


M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::ctr_get_prev_node(NodeBaseG& node) const
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent(node);

        int32_t parent_idx = node->parent_idx();

        if (parent_idx > 0)
        {
            return self.ctr_get_node_child(parent, parent_idx - 1);
        }
        else {
            NodeBaseG target_parent = ctr_get_prev_node(parent);

            if (target_parent.isSet())
            {
                int32_t node_size = self.ctr_get_node_size(target_parent, 0);
                return self.ctr_get_node_child(target_parent, node_size - 1);
            }
            else {
                return target_parent;
            }
        }
    }
    else {
        return NodeBaseG();
    }
}






#undef M_TYPE
#undef M_PARAMS

}}
