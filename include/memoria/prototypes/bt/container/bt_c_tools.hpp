
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

#include <memoria/core/container/macros.hpp>
#include <memoria/core/strings/string.hpp>

#include <memoria/prototypes/bt/nodes/branch_node_so.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ToolsName)

    using typename Base::Profile;
    using typename Base::BlockID;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::BranchNodeEntry;


public:
    TreeNodeConstPtr ctr_get_block(const BlockID& block_id) const
    {
        auto& self = this->self();
        return static_cast_block<TreeNodeConstPtr>(self.store().getBlock(block_id));
    }


    TreeNodeConstPtr ctr_get_root_node() const
    {
        auto& self = this->self();
        return self.ctr_get_block(self.root());
    }


    MEMORIA_V1_DECLARE_NODE_FN(DumpBlockSizesFn, ctr_dump_block_sizes);
    void ctr_dump_block_sizes(const TreeNodeConstPtr& node) const
    {
        return self().leaf_dispatcher().dispatch(node, DumpBlockSizesFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(MaxFn, max);
    BranchNodeEntry ctr_get_node_max_keys(const TreeNodeConstPtr& node) const
    {
        BranchNodeEntry entry;
        self().node_dispatcher().dispatch(node, MaxFn(), entry);
        return entry;
    }

    MEMORIA_V1_DECLARE_NODE_FN(GetSizesFn, sizes);
    Position ctr_get_node_sizes(const TreeNodeConstPtr& node) const
    {
        return self().node_dispatcher().dispatch(node, GetSizesFn());
    }

    Position ctr_get_leaf_sizes(const TreeNodeConstPtr& node) const
    {
        return self().leaf_dispatcher().dispatch(node, GetSizesFn());
    }

    MEMORIA_V1_DECLARE_NODE_FN(GetSizeFn, size);
    auto ctr_get_node_size(const TreeNodeConstPtr& node, int32_t stream) const
    {
        return self().node_dispatcher().dispatch(node, GetSizeFn(), stream);
    }

    MEMORIA_V1_DECLARE_NODE_FN(FindChildIdx, find_child_idx);
    auto ctr_find_child_idx(const TreeNodeConstPtr& node, const BlockID& child_id) const
    {
        return self().branch_dispatcher().dispatch(node, FindChildIdx(), child_id);
    }

    int32_t ctr_get_child_idx(const TreeNodeConstPtr& node, const BlockID& child_id) const
    {
        auto idx = ctr_find_child_idx(node, child_id);
        if (MMA_LIKELY(idx >= 0)) {
            return idx;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Requested child is not found in the b-tree: {}", child_id).do_throw();
        }
    }

    int32_t ctr_get_parent_idx(const TreePathT& path, size_t level) const
    {
        if (MMA_UNLIKELY(level + 1 >= path.size())) {
            return 0; // -1?
        }

        return self().ctr_get_child_idx(path[level + 1], path[level]->id());
    }

    void ctr_assign_path_nodes(const TreePathT& src, TreePathT& dst, size_t start_level = 0) const
    {
        if (src.size() != dst.size()) {
            dst.resize(src.size());
        }

        for (size_t ll = start_level; ll < src.size(); ll++) {
            dst[ll] = src[ll];
        }
    }

    MEMORIA_V1_DECLARE_NODE_FN(CheckCapacitiesFn, checkCapacities);
    bool ctr_check_node_capacities(const TreeNodeConstPtr& node, const Position& sizes) const
    {
        return self().node_dispatcher().dispatch(node, CheckCapacitiesFn(), sizes);
    }


    MEMORIA_V1_DECLARE_NODE_FN(GenerateDataEventsFn, generateDataEvents);
    void ctr_dump_node(const TreeNodeConstPtr& block, std::ostream& out = std::cout) const
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

    U8String ctr_describe_node(const TreeNodeConstPtr& block) const
    {
        std::stringstream ss;
        self().ctr_dump_node(block, ss);
        return ss.str();
    }

    void ctr_dump_path(TreePathT& path, size_t level, std::ostream& out = std::cout, int32_t depth = 100) const
    {
        auto& self = this->self();

        out << "Path:" << std::endl;

        for (size_t ll = level; ll < path.size(); ll++)
        {
            self.ctr_dump_node(path[ll], out);
        }
    }


protected:

    bool ctr_is_the_same_sode(const TreeNodeConstPtr& node1, const TreeNodeConstPtr& node2) const
    {
        return node1->id() == node2->id();
    }




    template <typename Node>
    TreeNodeConstPtr ctr_get_child_fn(Node&& node, int32_t idx) const
    {
        auto& self = this->self();
        return self.ctr_get_block(node.value(idx));
    }

    template <typename Node>
    TreeNodeConstPtr getLastChildFn(Node&& node) const
    {
        auto& self = this->self();
        auto size = node.size();
        return self.ctr_get_block(node.value(size - 1));
    }

    MEMORIA_V1_CONST_FN_WRAPPER(GetChildFn, ctr_get_child_fn);
    TreeNodeConstPtr ctr_get_node_child(const TreeNodeConstPtr& node, int32_t idx) const
    {
        auto result = self().branch_dispatcher().dispatch(node, GetChildFn(self()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Child must not be NULL").do_throw();
        }
    }

    MEMORIA_V1_CONST_FN_WRAPPER(GetLastChildFn, getLastChildFn);
    TreeNodeConstPtr ctr_get_node_last_child(const TreeNodeConstPtr& node) const
    {
        auto result = self().branch_dispatcher().dispatch(node, GetLastChildFn(self()));

        if (result.isSet())
        {
            return result;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Child must not be NULL").do_throw();
        }
    }



    MEMORIA_V1_CONST_FN_WRAPPER(GetChildForUpdateFn, getChildForUpdateFn);
    TreeNodePtr ctr_get_child_for_update(const TreeNodeConstPtr& node, int32_t idx) const
    {
        auto result = self().branch_dispatcher().dispatch(node, GetChildForUpdateFn(self()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Child must not be NULL").do_throw();
        }
    }

    MEMORIA_V1_DECLARE_NODE_FN(NodeStreamSizesFn, size_sums);
    Position ctr_node_stream_sizes(const TreeNodeConstPtr& node) const
    {
        return self().node_dispatcher().dispatch(node, NodeStreamSizesFn());
    }



    MEMORIA_V1_DECLARE_NODE_FN(SumsFn, sums);
    void ctr_sums(const TreeNodeConstPtr& node, int32_t start, int32_t end, BranchNodeEntry& sums) const
    {
        return self().branch_dispatcher().dispatch(node, SumsFn(), start, end, sums);
    }


    void ctr_sums(const TreeNodeConstPtr& node, const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        return self().leaf_dispatcher().dispatch(node, SumsFn(), start, end, sums);
    }

    template <typename Path>
    struct LeafSumsFn {
        template <typename Node, typename... Args>
        auto treeNode(Node&& node, Args&&... args) {
            return node.template leaf_sums<Path>(std::forward<Args>(args)...);
        }
    };

    struct LeafSizesFn {
        template <typename Node, typename... Args>
        auto treeNode(Node&& node, Args&&...) {
            return node.sizes();
        }
    };

    template <typename Path, typename... Args>
    auto ctr_leaf_sums(const TreeNodeConstPtr& node, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(node, LeafSumsFn<Path>(), std::forward<Args>(args)...);
    }


    auto ctr_leaf_sizes(const TreeNodeConstPtr& node) const
    {
        return self().leaf_dispatcher().dispatch(node, LeafSizesFn());
    }

public:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetINodeDataFn, value, BlockID);
    BlockID ctr_get_child_id(const TreeNodeConstPtr& node, int32_t idx) const
    {
        return self().branch_dispatcher().dispatch(node, GetINodeDataFn(), idx);
    }



    MEMORIA_V1_DECLARE_NODE_FN(LayoutNodeFn, layout);
    void ctr_layout_branch_node(const TreeNodePtr& node) const
    {
        return self().branch_dispatcher().dispatch(node, LayoutNodeFn());
    }

    void ctr_layout_leaf_node(const TreeNodePtr& node) const
    {
        return self().leaf_dispatcher().dispatch(node, LayoutNodeFn());
    }


protected:
    MEMORIA_V1_DECLARE_NODE_FN(GetActiveStreamsFn, active_streams);
    uint64_t ctr_get_active_streams(const TreeNodeConstPtr& node) const
    {
        return self().node_dispatcher().dispatch(node, GetActiveStreamsFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(IsNodeEmpty, is_empty);
    bool ctr_is_node_empty(const TreeNodePtr& node)
    {
        return self().node_dispatcher().dispatch(node, IsNodeEmpty());
    }


    MEMORIA_V1_DECLARE_NODE_FN(ForAllIDsFn, forAllValues);
    void ctr_for_all_ids(const TreeNodeConstPtr& node, int32_t start, int32_t end, std::function<void (const BlockID&)> fn) const
    {
        return self().branch_dispatcher().dispatch(node, ForAllIDsFn(), start, end, fn);
    }

    // TODO: errors handling
    void ctr_for_all_ids(const TreeNodeConstPtr& node, int32_t start, std::function<void (const BlockID&)> fn) const
    {
        return self().branch_dispatcher().dispatch(node, ForAllIDsFn(), start, fn);
    }

    void ctr_for_all_ids(const TreeNodeConstPtr& node, std::function<void (const BlockID&)> fn) const
    {
        return self().branch_dispatcher().dispatch(node, ForAllIDsFn(), fn);
    }


    struct SetChildIDFn {
        template <typename CtrT, typename T>
        BlockID treeNode(BranchNodeSO<CtrT, T>& node, int child_idx, const BlockID& child_id) const
        {
            auto old_value = node.value(child_idx);
            node.value(child_idx) = child_id;

            return old_value;
        }
    };

    BlockID ctr_set_child_id(const TreeNodePtr& node, int32_t child_idx, const BlockID& child_id)
    {
        self().ctr_update_block_guard(node);
        return self().branch_dispatcher().dispatch(node, SetChildIDFn(), child_idx, child_id);
    }



    struct GetBranchNodeChildernCount {
        template <typename CtrT, typename T, typename... Args>
        auto treeNode(BranchNodeSO<CtrT, T>& node, Args&&... args) const
        {
            return node->size();
        }
    };

    template <int32_t Stream>
    struct GetLeafNodeStreamSize {
        template <typename CtrT, typename T, typename... Args>
        auto treeNode(const LeafNodeSO<CtrT, T>& node, Args&&... args) const
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

    int32_t ctr_get_node_children_count(const TreeNodeConstPtr& node) const
    {
        return self().branch_dispatcher().dispatch(node, GetBranchNodeChildernCount());
    }



    int32_t ctr_get_branch_node_size(const TreeNodeConstPtr& node) const
    {
        return self().branch_dispatcher().dispatch(node, GetSizeFn());
    }

public:

    template <int32_t StreamIdx>
    int32_t ctr_get_leaf_stream_size(const TreeNodeConstPtr& node) const
    {
        return self().leaf_dispatcher().dispatch(node, GetLeafNodeStreamSize<StreamIdx>());
    }

    Position ctr_get_leaf_stream_sizes(const TreeNodeConstPtr& node) const
    {
        return self().leaf_dispatcher().dispatch(node, GetLeafNodeStreamSizes());
    }


    struct CreateIOVectorViewFn
    {
        template <typename T, typename... Args>
        std::unique_ptr<io::IOVector> treeNode(T&& node) const
        {
            return node->create_iovector_view();
        }
    };

    // TODO: error handling
    std::unique_ptr<io::IOVector> create_iovector_view(TreeNodePtr& node)
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

    // TODO: error handling
    void configure_iovector_view(TreeNodePtr& node, io::IOVector& io_vector)
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


    // TODO: error handling
    template <typename SubstreamPath>
    auto ctr_get_packed_struct(const TreeNodeConstPtr& leaf) const
    {
        return self().leaf_dispatcher().dispatch(leaf, GetPackedStructFn<SubstreamPath>());
    }

    template <typename SubstreamPath>
    auto ctr_get_packed_struct(TreeNodePtr& leaf)
    {
        return self().leaf_dispatcher().dispatch(leaf, GetPackedStructFn<SubstreamPath>());
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
