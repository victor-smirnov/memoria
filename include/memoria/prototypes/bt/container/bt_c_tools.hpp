
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
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::BranchNodeEntry;


public:
    Result<TreeNodePtr> ctr_get_block(const BlockID& block_id) const noexcept
    {
        auto& self = this->self();
        return static_cast_block<TreeNodePtr>(self.store().getBlock(block_id));
    }


    Result<TreeNodePtr> ctr_get_root_node() const noexcept
    {
        auto& self = this->self();
        return self.ctr_get_block(self.root());
    }


    MEMORIA_V1_DECLARE_NODE_FN(DumpBlockSizesFn, ctr_dump_block_sizes);
    VoidResult ctr_dump_block_sizes(const TreeNodePtr& node) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, DumpBlockSizesFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(MaxFn, max);
    Result<BranchNodeEntry> ctr_get_node_max_keys(const TreeNodePtr& node) const noexcept
    {
        BranchNodeEntry entry;
        MEMORIA_TRY_VOID(self().node_dispatcher().dispatch(node, MaxFn(), entry));
        return Result<BranchNodeEntry>::of(entry);
    }

    MEMORIA_V1_DECLARE_NODE_FN(GetSizesFn, sizes);
    Result<Position> ctr_get_node_sizes(const TreeNodePtr& node) const noexcept
    {
        return self().node_dispatcher().dispatch(node, GetSizesFn());
    }

    Result<Position> ctr_get_leaf_sizes(const TreeNodePtr& node) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, GetSizesFn());
    }

    MEMORIA_V1_DECLARE_NODE_FN(GetSizeFn, size);
    auto ctr_get_node_size(const TreeNodePtr& node, int32_t stream) const noexcept
    {
        return self().node_dispatcher().dispatch(node, GetSizeFn(), stream);
    }

    MEMORIA_V1_DECLARE_NODE_FN(FindChildIdx, find_child_idx);
    auto ctr_find_child_idx(const TreeNodePtr& node, const BlockID& child_id) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, FindChildIdx(), child_id);
    }

    Int32Result ctr_get_child_idx(const TreeNodePtr& node, const BlockID& child_id) const noexcept
    {
        MEMORIA_TRY(idx, ctr_find_child_idx(node, child_id));
        if (MMA_LIKELY(idx >= 0)) {
            return Int32Result::of(idx);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Requested child is not found in the b-tree: {}", child_id);
        }
    }

    Int32Result ctr_get_parent_idx(const TreePathT& path, size_t level) const noexcept
    {
        if (MMA_UNLIKELY(level + 1 >= path.size())) {
            return Int32Result::of(0); // -1?
        }

        return self().ctr_get_child_idx(path[level + 1], path[level]->id());
    }

    VoidResult ctr_assign_path_nodes(const TreePathT& src, TreePathT& dst, size_t start_level = 0) const noexcept
    {
        if (src.size() != dst.size()) {
            dst.resize(src.size());
        }

        for (size_t ll = start_level; ll < src.size(); ll++) {
            dst[ll] = src[ll];
        }

        return VoidResult::of();
    }

    // TODO: check noexcept
    MEMORIA_V1_DECLARE_NODE_FN(CheckCapacitiesFn, checkCapacities);
    BoolResult ctr_check_node_capacities(const TreeNodePtr& node, const Position& sizes) const noexcept
    {
        return self().node_dispatcher().dispatch(node, CheckCapacitiesFn(), sizes);
    }


    MEMORIA_V1_DECLARE_NODE_FN(GenerateDataEventsFn, generateDataEvents);
    VoidResult ctr_dump_node(const TreeNodePtr& block, std::ostream& out = std::cout) const noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            const auto& self = this->self();
            if (block)
            {
                TextBlockDumper dumper(out);

                MEMORIA_TRY_VOID(self.node_dispatcher().dispatch(block, GenerateDataEventsFn(), &dumper));

                out<<std::endl;
                out<<std::endl;
            }
            else {
                out << "NULL" << std::endl;
            }

            return VoidResult::of();
        });
    }

    VoidResult ctr_dump_path(TreePathT& path, size_t level, std::ostream& out = std::cout, int32_t depth = 100) const noexcept
    {
        auto& self = this->self();

        out << "Path:" << std::endl;

        for (size_t ll = level; ll < path.size(); ll++)
        {
            MEMORIA_TRY_VOID(self.ctr_dump_node(path[ll], out));
        }

        return VoidResult::of();
    }


protected:

    bool ctr_is_the_same_sode(const TreeNodePtr& node1, const TreeNodePtr& node2) const noexcept
    {
        return node1->id() == node2->id();
    }




    template <typename Node>
    Result<TreeNodePtr> ctr_get_child_fn(Node&& node, int32_t idx) const noexcept
    {
        auto& self = this->self();
        return self.ctr_get_block(node.value(idx));
    }

    template <typename Node>
    Result<TreeNodePtr> getLastChildFn(Node&& node) const noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(size, node.size());
        return self.ctr_get_block(node.value(size - 1));
    }

    MEMORIA_V1_CONST_FN_WRAPPER(GetChildFn, ctr_get_child_fn);
    Result<TreeNodePtr> ctr_get_node_child(const TreeNodePtr& node, int32_t idx) const noexcept
    {
        using ResultT = Result<TreeNodePtr>;
        ResultT result = self().branch_dispatcher().dispatch(node, GetChildFn(self()), idx);

        if (result.get().isSet())
        {
            return result;
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Child must not be NULL");
        }
    }

    MEMORIA_V1_CONST_FN_WRAPPER(GetLastChildFn, getLastChildFn);
    Result<TreeNodePtr> ctr_get_node_last_child(const TreeNodePtr& node) const noexcept
    {
        using ResultT = Result<TreeNodePtr>;
        ResultT result = self().branch_dispatcher().dispatch(node, GetLastChildFn(self()));

        if (result.get().isSet())
        {
            return result;
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Child must not be NULL");
        }
    }



    MEMORIA_V1_CONST_FN_WRAPPER(GetChildForUpdateFn, getChildForUpdateFn);
    Result<TreeNodePtr> ctr_get_child_for_update(const TreeNodePtr& node, int32_t idx) const noexcept
    {
        using ResultT = Result<TreeNodePtr>;
        ResultT result = self().branch_dispatcher().dispatch(node, GetChildForUpdateFn(self()), idx);

        if (result.get().isSet())
        {
            return result;
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Child must not be NULL");
        }
    }

    MEMORIA_V1_DECLARE_NODE_FN(NodeStreamSizesFn, size_sums);
    Result<Position> ctr_node_stream_sizes(const TreeNodePtr& node) const noexcept
    {
        return self().node_dispatcher().dispatch(node, NodeStreamSizesFn());
    }



    MEMORIA_V1_DECLARE_NODE_FN(SumsFn, sums);
    VoidResult ctr_sums(const TreeNodePtr& node, int32_t start, int32_t end, BranchNodeEntry& sums) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, SumsFn(), start, end, sums);
    }


    VoidResult ctr_sums(const TreeNodePtr& node, const Position& start, const Position& end, BranchNodeEntry& sums) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, SumsFn(), start, end, sums);
    }


    // TODO: noexcept
    template <typename Path>
    struct LeafSumsFn {
        template <typename Node, typename... Args>
        auto treeNode(Node&& node, Args&&... args) noexcept {
            return node.template leaf_sums<Path>(std::forward<Args>(args)...);
        }
    };

    struct LeafSizesFn {
        template <typename Node, typename... Args>
        auto treeNode(Node&& node, Args&&...) noexcept {
            return node.sizes();
        }
    };

    template <typename Path, typename... Args>
    auto ctr_leaf_sums(const TreeNodePtr& node, Args&&... args) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, LeafSumsFn<Path>(), std::forward<Args>(args)...);
    }


    auto ctr_leaf_sizes(const TreeNodePtr& node) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, LeafSizesFn());
    }

public:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetINodeDataFn, value, BlockID);
    Result<BlockID> ctr_get_child_id(const TreeNodePtr& node, int32_t idx) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, GetINodeDataFn(), idx);
    }



    MEMORIA_V1_DECLARE_NODE_FN(LayoutNodeFn, layout);
    VoidResult ctr_layout_branch_node(TreeNodePtr& node, uint64_t active_streams) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, LayoutNodeFn(), active_streams);
    }

    VoidResult ctr_layout_leaf_node(TreeNodePtr& node, const Position& sizes) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, LayoutNodeFn(), sizes);
    }


protected:
    MEMORIA_V1_DECLARE_NODE_FN(GetActiveStreamsFn, active_streams);
    Result<uint64_t> ctr_get_active_streams(const TreeNodePtr& node) const noexcept
    {
        return self().node_dispatcher().dispatch(node, GetActiveStreamsFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(IsNodeEmpty, is_empty);
    BoolResult ctr_is_node_empty(const TreeNodePtr& node) noexcept
    {
        return self().node_dispatcher().dispatch(node, IsNodeEmpty());
    }


    MEMORIA_V1_DECLARE_NODE_FN(ForAllIDsFn, forAllValues);
    VoidResult ctr_for_all_ids(const TreeNodePtr& node, int32_t start, int32_t end, std::function<VoidResult (const BlockID&)> fn) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, ForAllIDsFn(), start, end, fn);
    }

    // TODO: errors handling
    VoidResult ctr_for_all_ids(const TreeNodePtr& node, int32_t start, std::function<VoidResult (const BlockID&)> fn) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, ForAllIDsFn(), start, fn);
    }

    VoidResult ctr_for_all_ids(const TreeNodePtr& node, std::function<VoidResult (const BlockID&)> fn) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, ForAllIDsFn(), fn);
    }


    struct SetChildIDFn {
        template <typename CtrT, typename T>
        Result<BlockID> treeNode(BranchNodeSO<CtrT, T>& node, int child_idx, const BlockID& child_id) const noexcept
        {
            using ResultT = Result<BlockID>;

            auto old_value = node.value(child_idx);
            node.value(child_idx) = child_id;

            return ResultT::of(old_value);
        }
    };

    Result<BlockID> ctr_set_child_id(TreeNodePtr& node, int32_t child_idx, const BlockID& child_id) noexcept
    {
        MEMORIA_TRY_VOID(self().ctr_update_block_guard(node));
        return self().branch_dispatcher().dispatch(node, SetChildIDFn(), child_idx, child_id);
    }



    struct GetBranchNodeChildernCount {
        template <typename CtrT, typename T, typename... Args>
        Int32Result treeNode(BranchNodeSO<CtrT, T>& node, Args&&... args) const noexcept
        {
            return node->size();
        }
    };

    template <int32_t Stream>
    struct GetLeafNodeStreamSize {
        template <typename CtrT, typename T, typename... Args>
        Int32Result treeNode(const LeafNodeSO<CtrT, T>& node, Args&&... args) const noexcept
        {
            return node.template streamSize<Stream>();
        }
    };

    struct GetLeafNodeStreamSizes {
        template <typename CtrT, typename T, typename... Args>
        Result<Position> treeNode(const LeafNodeSO<CtrT, T>& node, Args&&... args) const noexcept
        {
            return node.sizes();
        }
    };

    Int32Result ctr_get_node_children_count(const TreeNodePtr& node) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, GetBranchNodeChildernCount());
    }



    Int32Result ctr_get_branch_node_size(const TreeNodePtr& node) const noexcept
    {
        return self().branch_dispatcher().dispatch(node, GetSizeFn());
    }

public:

    template <int32_t StreamIdx>
    Int32Result ctr_get_leaf_stream_size(const TreeNodePtr& node) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, GetLeafNodeStreamSize<StreamIdx>());
    }

    Result<Position> ctr_get_leaf_stream_sizes(const TreeNodePtr& node) const noexcept
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
    std::unique_ptr<io::IOVector> create_iovector_view(TreeNodePtr& node) noexcept
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
    VoidResult configure_iovector_view(TreeNodePtr& node, io::IOVector& io_vector) noexcept
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
    auto ctr_get_packed_struct(const TreeNodePtr& leaf) const noexcept
    {
        return self().leaf_dispatcher().dispatch(leaf, GetPackedStructFn<SubstreamPath>());
    }

    template <typename SubstreamPath>
    auto ctr_get_packed_struct(TreeNodePtr& leaf) noexcept
    {
        return self().leaf_dispatcher().dispatch(leaf, GetPackedStructFn<SubstreamPath>());
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
