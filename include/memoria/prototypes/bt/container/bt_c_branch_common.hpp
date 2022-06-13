
// Copyright 2015-2021 Victor Smirnov
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/packed/tools/packed_allocator_types.hpp>


#include <memoria/prototypes/bt/nodes/branch_node.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchCommonName)
public:
    using typename Base::TreeNodeConstPtr;
    using typename Base::TreeNodePtr;
    using typename Base::TreePathT;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockID;


public:

    using SplitFn = std::function<void (const TreeNodePtr&, const TreeNodePtr&)>;


    struct SetChildIDFn {
        template <typename CtrT, typename T>
        BlockID treeNode(BranchNodeSO<CtrT, T>& node, size_t child_idx, const BlockID& child_id) const
        {
            auto old_value = node.value(child_idx);
            node.value(child_idx) = child_id;

            return old_value;
        }
    };

    BlockID ctr_set_child_id(const TreeNodePtr& node, size_t child_idx, const BlockID& child_id)
    {
        self().ctr_update_block_guard(node);
        return self().branch_dispatcher().dispatch(node, SetChildIDFn(), child_idx, child_id);
    }

    void ctr_create_new_root_block(TreePathT& path);

    MEMORIA_V1_DECLARE_NODE_FN(GetNonLeafCapacityFn, capacity);
    size_t ctr_get_branch_node_capacity(const TreeNodeConstPtr& node) const
    {
        return self().branch_dispatcher().dispatch(node, GetNonLeafCapacityFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(SplitNodeFn, split_to);
    void ctr_split_branch_node(const TreeNodePtr& src, const TreeNodePtr& tgt, size_t split_at);

    MEMORIA_V1_DECLARE_NODE_FN(TryMergeNodesFn, merge_with);
    bool ctr_try_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level);
    bool ctr_merge_branch_nodes(TreePathT& tgt_path, TreePathT& src_path, size_t level, bool only_if_same_parent = false);

    MEMORIA_V1_DECLARE_NODE_FN(InsertFn, insert);
    PkdUpdateStatus ctr_insert_to_branch_node(
            TreePathT& path,
            size_t level,
            size_t idx,
            const BranchNodeEntry& keys,
            const BlockID& id
    );

    void ctr_split_path(
            TreePathT& path,
            size_t level,
            size_t split_at
    );

    void ctr_split_path_raw(
            TreePathT& path,
            size_t level,
            size_t split_at
    );

    void ctr_split_node(
            TreePathT& path,
            size_t level,
            const SplitFn& split_fn
    )
    {
        ctr_split_node_raw(path, level, split_fn);
        return self().ctr_expect_prev_node(path, level);
    }

    void ctr_split_node_raw(
            TreePathT& path,
            size_t level,
            const SplitFn& split_fn
    );

    MEMORIA_V1_DECLARE_NODE_FN(RemoveSpaceFn, try_remove_entries);
    void ctr_remove_branch_content(TreePathT& path, size_t level, size_t start, size_t end);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
PkdUpdateStatus M_TYPE::ctr_insert_to_branch_node(
        TreePathT& path,
        size_t level,
        size_t idx,
        const BranchNodeEntry& sums,
        const BlockID& id
)
{
        auto& self = this->self();

        self.ctr_cow_clone_path(path, level);

        self.ctr_update_block_guard(path[level]);
        TreeNodeConstPtr node = path[level];

        PkdUpdateStatus status = self.branch_dispatcher().dispatch(node.as_mutable(), InsertFn(), idx, sums, id);
        if (is_success(status)) {
            if (!node->is_root())
            {
                self.ctr_update_path(path, level);
            }
        }

        return status;
}


M_PARAMS
void M_TYPE::ctr_split_branch_node(const TreeNodePtr& src, const TreeNodePtr& tgt, size_t split_at)
{
    auto& self = this->self();
    return self.branch_dispatcher().dispatch(src, tgt, SplitNodeFn(), split_at);
}


M_PARAMS
void M_TYPE::ctr_create_new_root_block(TreePathT& path)
{
    auto& self = this->self();

    self.ctr_cow_clone_path(path, 0);

    TreeNodeConstPtr root = path.root();

    auto new_root = self.ctr_create_node(root->level() + 1, true, false, root->header().memory_block_size());

    self.ctr_copy_root_metadata(root, new_root);
    self.ctr_root_to_node(root.as_mutable());

    auto max = self.ctr_get_node_max_keys(root);

    path.add_root(new_root.as_immutable());

    assert_success(self.ctr_insert_to_branch_node(path, new_root->level(), 0, max, root->id()));

    self.ctr_ref_block(root->id());

    return self.set_root(new_root->id());
}

M_PARAMS
bool M_TYPE::ctr_try_merge_branch_nodes(TreePathT& tgt_path, const TreePathT& src_path, size_t level)
{
    auto& self = this->self();

    self.ctr_check_same_paths(tgt_path, src_path, level + 1);

    auto parent_idx = self.ctr_get_parent_idx(src_path, level);
    self.ctr_cow_clone_path(tgt_path, level);

    TreeNodeConstPtr src = src_path[level];
    TreeNodeConstPtr tgt = tgt_path[level];

    self.ctr_update_block_guard(tgt);

    auto res = self.branch_dispatcher().dispatch_1st_const(src, tgt.as_mutable(), TryMergeNodesFn());
    if (!is_success(res)) {
        return false;
    }

    // FIXME: in case of 'packed error' (that shouldn't happen),
    // the tree is in inconsistent state. So we need to backup the parent node too.
    self.ctr_remove_branch_node_entry(tgt_path, level + 1, parent_idx);

    // FIXME. Here src is actually immutable.
    // So ctr_cow_ref_children_after_merge should take const block ptr.

    self.ctr_cow_ref_children_after_merge(src.as_mutable());

    self.ctr_update_path(tgt_path, level);
    self.ctr_unref_block(src->id());

    return true;
}


M_PARAMS
bool M_TYPE::ctr_merge_branch_nodes(
        TreePathT& tgt_path,
        TreePathT& src_path,
        size_t level,
        bool only_if_same_parent
)
{
    auto& self = this->self();

    auto is_same_parent = self.ctr_is_the_same_parent(tgt_path, src_path, level);
    if (is_same_parent)
    {
        auto merged = self.ctr_try_merge_branch_nodes(tgt_path, src_path, level);
        if (!merged)
        {
            self.ctr_assign_path_nodes(tgt_path, src_path, level);
            self.ctr_expect_next_node(src_path, level);
        }

        return merged;
    }
    else if (!only_if_same_parent)
    {
        auto merged = ctr_merge_branch_nodes(tgt_path, src_path, level + 1);
        if (merged)
        {
            self.ctr_assign_path_nodes(tgt_path, src_path, level);
            self.ctr_expect_next_node(src_path, level);

            return self.ctr_try_merge_branch_nodes(tgt_path, src_path, level);
        }
        else
        {
            self.ctr_assign_path_nodes(tgt_path, src_path, level);
            self.ctr_expect_next_node(src_path, level);
        }
    }

    return false;
}




M_PARAMS
void M_TYPE::ctr_split_node_raw(
        TreePathT& path,
        size_t level,
        const SplitFn& split_fn
)
{
    auto& self = this->self();

    if (level + 1 == path.size())
    {
        self.ctr_create_new_root_block(path);
    }

    TreeNodeConstPtr left_node = path[level];
    // FIXME: must update the node here!!!

    auto right_node = self.ctr_create_node(level, false, left_node->is_leaf(), left_node->header().memory_block_size());

    split_fn(left_node.as_mutable(), right_node);

    auto right_max = self.ctr_get_node_max_keys(right_node.as_immutable());

    self.ctr_update_path(path, level);

    auto new_parent_idx = self.ctr_get_child_idx(path[level + 1], path[level]->id());

    PkdUpdateStatus insertion_status = self.ctr_insert_to_branch_node(path, level + 1, new_parent_idx + 1, right_max, right_node->id());
    if (!is_success(insertion_status))
    {
        auto parent_size = self.ctr_get_node_size(path[level + 1], 0);
        size_t parent_split_idx = parent_size / 2;

        ctr_split_path_raw(path, level + 1, parent_split_idx);

        if (new_parent_idx < parent_split_idx)
        {
            TreePathT left_path(path, level + 1);
            self.ctr_expect_prev_node(left_path, level + 1);

            self.ctr_assign_path_nodes(
                        left_path,
                        path,
                        level + 1
                        );
        }
        else {
            new_parent_idx -= parent_split_idx;
        }

        PkdUpdateStatus right_path_insertion_status =
                self.ctr_insert_to_branch_node(
                    path,
                    level + 1,
                    new_parent_idx + 1,
                    right_max,
                    right_node->id()
                );


        if (!is_success(right_path_insertion_status))
        {
            MEMORIA_MAKE_GENERIC_ERROR("Can't insert node into the right path").do_throw();
        }
    }

    path[level] = right_node.as_immutable();
    self.ctr_ref_block(right_node->id());

    self.ctr_check_path(path, level);
}

M_PARAMS
void M_TYPE::ctr_split_path(
        TreePathT& path,
        size_t level,
        size_t split_at
)
{
    // it is assumed that level > 0

    auto& self = this->self();

    return ctr_split_node(
                path,
                level,
                [&self, split_at](const TreeNodePtr& left, const TreeNodePtr& right)
    {
        self.ctr_split_branch_node(left, right, split_at);
    });
}

M_PARAMS
void M_TYPE::ctr_split_path_raw(
        TreePathT& path,
        size_t level,
        size_t split_at
)
{
    auto& self = this->self();
    return ctr_split_node_raw(
                path,
                level,
                [&self, split_at](const TreeNodePtr& left, const TreeNodePtr& right)
    {
        self.ctr_split_branch_node(left, right, split_at);
    });
}

M_PARAMS
void M_TYPE::ctr_remove_branch_content(TreePathT& path, size_t level, size_t start, size_t end)
{
    auto& self = this->self();

    self.ctr_cow_clone_path(path, level);
    self.ctr_update_block_guard(path[level]);

    self.ctr_for_all_ids(path[level], start, end, [&](const BlockID& id) {
        return self.ctr_unref_block(id);
    });

    PkdUpdateStatus status = self.branch_dispatcher().dispatch(path[level].as_mutable(), RemoveSpaceFn(), start, end);
    assert_success(status);
    self.ctr_update_path(path, level);
}

#undef M_TYPE
#undef M_PARAMS

}
