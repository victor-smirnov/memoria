
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchFixedName)

public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

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
    bool ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry);

    void ctr_update_branch_nodes(NodeBaseG& node, int32_t& idx, const BranchNodeEntry& entry);





    MEMORIA_V1_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);
    bool ctr_can_merge_nodes(const NodeBaseG& tgt, const NodeBaseG& src)
    {
        return self().node_dispatcher().dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(MergeNodesFn, mergeWith, OpStatus);
    void ctr_do_merge_branch_nodes(NodeBaseG& tgt, NodeBaseG& src);
    bool ctr_merge_branch_nodes(NodeBaseG& tgt, NodeBaseG& src);
    bool ctr_merge_current_branch_nodes(NodeBaseG& tgt, NodeBaseG& src);


MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
OpStatus M_TYPE::ctr_insert_to_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys, const BlockID& id)
{
    auto& self = this->self();

    self.ctr_update_block_guard(node);
    if (isFail(self.branch_dispatcher().dispatch(node, InsertFn(), idx, keys, id))) {
        return OpStatus::FAIL;
    }

    self.ctr_update_children(node, idx);

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);
        auto max = self.ctr_get_node_max_keys(node);
        self.ctr_update_branch_nodes(parent, node->parent_idx(), max);
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

    self.ctr_update_block_guard(left_node);
    NodeBaseG left_parent = self.ctr_get_node_parent_for_update(left_node);

    NodeBaseG right_node = self.ctr_create_node(left_node->level(), false, left_node->is_leaf(), left_node->header().memory_block_size());

    split_fn(left_node, right_node);

    auto left_max  = self.ctr_get_node_max_keys(left_node);
    auto right_max = self.ctr_get_node_max_keys(right_node);

    int32_t parent_idx = left_node->parent_idx();

    self.ctr_update_branch_nodes(left_parent, parent_idx, left_max);

    if (self.ctr_get_branch_node_capacity(left_parent, -1) > 0)
    {
        OOM_THROW_IF_FAILED(self.ctr_insert_to_branch_node(left_parent, parent_idx + 1, right_max, right_node->id()), MMA1_SRC);
    }
    else {
        NodeBaseG right_parent = ctr_split_path(left_parent, parent_idx + 1);

        OOM_THROW_IF_FAILED(self.ctr_insert_to_branch_node(right_parent, 0, right_max, right_node->id()), MMA1_SRC);
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
bool M_TYPE::ctr_update_branch_node(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys)
{
    self().ctr_update_block_guard(node);
    OOM_THROW_IF_FAILED(self().branch_dispatcher().dispatch(node, UpdateNodeFn(), idx, keys), MMA1_SRC);
    return true;
}




M_PARAMS
void M_TYPE::ctr_update_branch_nodes(NodeBaseG& node, int32_t& idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    NodeBaseG tmp = node;

    self.ctr_update_branch_node(tmp, idx, entry);

    while(!tmp->is_root())
    {
        auto max = self.ctr_get_node_max_keys(tmp);
        int32_t parent_idx = tmp->parent_idx();

        tmp = self.ctr_get_node_parent_for_update(tmp);

        self.ctr_update_branch_node(tmp, parent_idx, max);
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







/**
 * \brief Merge *src* path to the *tgt* path unconditionally.
 *
 * Perform merging of two paths, *src* to *dst* at the specified *level*. Both nodes (at boths paths) must
 * have the same parent.
 *
 * \param tgt path to node to be merged with
 * \param src path to node to be merged
 * \param level level of the node in the tree
 * \return true if paths have been merged
 *
 * \see mergeWithSiblings - this is the basic method
 */

M_PARAMS
void M_TYPE::ctr_do_merge_branch_nodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    self.ctr_update_block_guard(tgt);
    self.ctr_update_block_guard(src);

    int32_t tgt_size = self.ctr_get_node_size(tgt, 0);

    OOM_THROW_IF_FAILED(self.branch_dispatcher().dispatch(src, tgt, MergeNodesFn()), MMA1_SRC);

    self.ctr_update_children(tgt, tgt_size);

    NodeBaseG src_parent = self.ctr_get_node_parent(src);
    int32_t parent_idx       = src->parent_idx();

    MEMORIA_V1_ASSERT(parent_idx, >, 0);

    auto max = self.ctr_get_node_max_keys(tgt);

    OOM_THROW_IF_FAILED(self.ctr_remove_non_leaf_node_entry(src_parent, parent_idx), MMA1_SRC);

    int32_t idx = parent_idx - 1;

    self.ctr_update_branch_nodes(src_parent, idx, max);

    self.store().removeBlock(src->id());
}

/**
 * \brief Merge *src* path to the *tgt* path.
 *
 * Merge two tree paths, *src* to *dst* upward starting from nodes specified with *level*. If both these
 * nodes have different parents, then recursively merge parents first. Calls \ref ctr_can_merge_nodes to check if nodes can be merged.
 * This call will try to merge parents only if current nodes can be merged.
 *
 * If after nodes have been merged the resulting path is redundant, that means it consists from a single node chain,
 * then this path is truncated from the tree root down to the specified *level*.
 *
 * Unlike this call, \ref mergePaths tries to merge paths starting from the root down to the specified *level*.
 *
 * \param tgt path to node to be merged with
 * \param src path to node to be merged
 * \param level level of the node in the tree
 * \return true if paths have been merged
 *
 * \see mergeWithSiblings - this is the basic method
 * \see ctr_can_merge_nodes, removeRedundantRoot, mergeNodes, ctr_is_the_same_parent
 */

M_PARAMS
bool M_TYPE::ctr_merge_branch_nodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    if (self.ctr_can_merge_nodes(tgt, src))
    {
        if (self.ctr_is_the_same_parent(tgt, src))
        {
            self.ctr_do_merge_branch_nodes(tgt, src);

            self.ctr_remove_redundant_root(tgt);

            return true;
        }
        else
        {
            NodeBaseG tgt_parent = self.ctr_get_node_parent(tgt);
            NodeBaseG src_parent = self.ctr_get_node_parent(src);

            if (ctr_merge_branch_nodes(tgt_parent, src_parent))
            {
                self.ctr_do_merge_branch_nodes(tgt, src);

                self.ctr_remove_redundant_root(tgt);

                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
}



M_PARAMS
bool M_TYPE::ctr_merge_current_branch_nodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    if (self.ctr_can_merge_nodes(tgt, src))
    {
        self.ctr_do_merge_branch_nodes(tgt, src);

        self.ctr_remove_redundant_root(tgt);

        return true;
    }
    else
    {
        return false;
    }
}




#undef M_TYPE
#undef M_PARAMS

}}
