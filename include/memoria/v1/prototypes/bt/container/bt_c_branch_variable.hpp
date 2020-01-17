
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchVariableName)

public:
    using Types = typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;
    using typename Base::BlockID;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;

    typedef std::function<void (NodeBaseG&, NodeBaseG&)>                        SplitFn;

    static const int32_t Streams = Types::Streams;

public:
    void ctr_update_path(const NodeBaseG& node);

public:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(InsertFn, insert, OpStatus);
    OpStatus ctr_insert_to_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys, const BlockID& id);

    NodeBaseG ctr_split_path(NodeBaseG& node, int32_t split_at);
    NodeBaseG ctr_split_node(NodeBaseG& node, SplitFn split_fn);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(UpdateNodeFn, updateUp, OpStatus);


    MMA1_NODISCARD bool ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry);

    void ctr_update_branch_nodes(NodeBaseG& node, int32_t& idx, const BranchNodeEntry& entry);

    void ctr_update_branch_nodes_no_backup(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(TryMergeNodesFn, mergeWith, OpStatus);
    bool ctr_try_merge_branch_nodes(NodeBaseG& tgt, NodeBaseG& src);
    bool ctr_merge_branch_nodes(NodeBaseG& tgt, NodeBaseG& src);
    bool ctr_merge_current_branch_nodes(NodeBaseG& tgt, NodeBaseG& src);


MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
OpStatus M_TYPE::ctr_insert_to_branch_node(
        NodeBaseG& node,
        int32_t idx,
        const BranchNodeEntry& sums,
        const BlockID& id
)
{
    auto& self = this->self();

    self.ctr_update_block_guard(node);

    if(isFail(self.branch_dispatcher().dispatch(node, InsertFn(), idx, sums, id))) {
        return OpStatus::FAIL;
    }

    self.ctr_update_children(node, idx);

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);
        int32_t parent_idx = node->parent_idx();

        auto max = self.ctr_get_node_max_keys(node);
        self.ctr_update_branch_nodes(parent, parent_idx, max);
    }

    return OpStatus::OK;
}




M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::ctr_split_node(NodeBaseG& left_node, SplitFn split_fn)
{
    auto& self = this->self();

    if (left_node->is_root())
    {
        self.ctr_create_new_root_block(left_node);
    }
    else {
        self.ctr_update_block_guard(left_node);
    }

    NodeBaseG left_parent = self.ctr_get_node_parent_for_update(left_node);

    NodeBaseG right_node = self.ctr_create_node(left_node->level(), false, left_node->is_leaf(), left_node->header().memory_block_size());

    split_fn(left_node, right_node);

    auto left_max  = self.ctr_get_node_max_keys(left_node);
    auto right_max = self.ctr_get_node_max_keys(right_node);

    int32_t parent_idx = left_node->parent_idx();

    self.ctr_update_branch_nodes(left_parent, parent_idx, left_max);

    BlockUpdateMgr mgr(self);
    mgr.add(left_parent);

    if(isFail(self.ctr_insert_to_branch_node(left_parent, parent_idx + 1, right_max, right_node->id())))
    {
        mgr.rollback();

        NodeBaseG right_parent = ctr_split_path(left_parent, parent_idx + 1);

        mgr.add(right_parent);

        if(isFail(self.ctr_insert_to_branch_node(right_parent, 0, right_max, right_node->id())))
        {
            mgr.rollback();

            int32_t right_parent_size = self.ctr_get_node_size(right_parent, 0);

            ctr_split_path(right_parent, right_parent_size / 2);

            OOM_THROW_IF_FAILED(self.ctr_insert_to_branch_node(right_parent, 0, right_max, right_node->id()), MMA1_SRC);
        }
    }

    return right_node;
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::ctr_split_path(NodeBaseG& left_node, int32_t split_at)
{
    auto& self = this->self();

    return ctr_split_node(left_node, [&self, split_at](NodeBaseG& left, NodeBaseG& right){
        return self.ctr_split_branch_node(left, right, split_at);
    });
}



M_PARAMS
bool M_TYPE::ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    BlockUpdateMgr mgr(self);
    mgr.add(node);

    if (isFail(self.branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, entry))) {
        mgr.rollback();
        return false;
    }

    return true;
}





M_PARAMS
void M_TYPE::ctr_update_branch_nodes(NodeBaseG& node, int32_t& idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    self.ctr_update_block_guard(node);

    if (!self.ctr_update_branch_node(node, idx, entry))
    {
        int32_t size        = self.ctr_get_node_size(node, 0);
        int32_t split_idx   = size / 2;

        NodeBaseG right = self.ctr_split_path(node, split_idx);

        if (idx >= split_idx)
        {
            idx -= split_idx;
            node = right;
        }

        bool result = self.ctr_update_branch_node(node, idx, entry);
        MEMORIA_V1_ASSERT_TRUE(result);
    }

    if(!node->is_root())
    {
        int32_t parent_idx = node->parent_idx();
        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);

        auto max = self.ctr_get_node_max_keys(node);
        self.ctr_update_branch_nodes(parent, parent_idx, max);
    }
}






M_PARAMS
void M_TYPE::ctr_update_path(const NodeBaseG& node)
{
    auto& self = this->self();

    if (!node->is_root())
    {
        auto entry = self.ctr_get_node_max_keys(node);

        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);
        int32_t parent_idx = node->parent_idx();
        self.ctr_update_branch_nodes(parent, parent_idx, entry);
    }
}



M_PARAMS
void M_TYPE::ctr_update_branch_nodes_no_backup(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    self.ctr_update_branch_node(node, idx, entry);

    if(!node->is_root())
    {
        int32_t parent_idx = node->parent_idx();
        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);

        self.ctr_update_branch_nodes_no_backup(parent, parent_idx, entry);
    }
}


M_PARAMS
bool M_TYPE::ctr_try_merge_branch_nodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    BlockUpdateMgr mgr(self);

    self.ctr_update_block_guard(src);
    self.ctr_update_block_guard(tgt);

    mgr.add(src);
    mgr.add(tgt);


    int32_t tgt_size            = self.ctr_get_node_size(tgt, 0);
    NodeBaseG src_parent    = self.ctr_get_node_parent(src);
    int32_t parent_idx          = src->parent_idx();

    MEMORIA_V1_ASSERT(parent_idx, >, 0);

    if (isFail(self.branch_dispatcher().dispatch(src, tgt, TryMergeNodesFn())))
    {
        mgr.rollback();
        return false;
    }

    self.ctr_update_children(tgt, tgt_size);

    auto max = self.ctr_get_node_max_keys(tgt);

    if (isFail(self.ctr_remove_non_leaf_node_entry(src_parent, parent_idx))) {
        mgr.rollback();
        return false;
    }

    int32_t idx = parent_idx - 1;

    self.ctr_update_branch_nodes(src_parent, idx, max);

    self.store().removeBlock(src->id()).terminate_if_error();

    return true;
}


M_PARAMS
bool M_TYPE::ctr_merge_branch_nodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    if (self.ctr_is_the_same_parent(tgt, src))
    {
        return self.ctr_merge_current_branch_nodes(tgt, src);
    }
    else
    {
        NodeBaseG tgt_parent = self.ctr_get_node_parent(tgt);
        NodeBaseG src_parent = self.ctr_get_node_parent(src);

        if (ctr_merge_branch_nodes(tgt_parent, src_parent))
        {
            return self.ctr_merge_current_branch_nodes(tgt, src);
        }
        else
        {
            return false;
        }
    }
}




M_PARAMS
bool M_TYPE::ctr_merge_current_branch_nodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    if (self.ctr_try_merge_branch_nodes(tgt, src))
    {
        self.ctr_remove_redundant_root(tgt);
        return true;
    }
    else {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS

}}
