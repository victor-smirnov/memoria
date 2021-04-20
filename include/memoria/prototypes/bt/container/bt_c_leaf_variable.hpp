
// Copyright 2015 Victor Smirnov
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

#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>
#include <memoria/prototypes/bt/nodes/branch_node_so.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::LeafVariableName)

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;

    // TODO: noexcept
    template <int32_t Stream>
    struct InsertStreamEntryFn
    {
        template <
            int32_t Idx,
            typename SubstreamType,
            typename Entry
        >
        VoidResult stream(SubstreamType&& obj, int32_t idx, const Entry& entry) noexcept
        {
            if (obj)
            {
                return obj.insert_entries(idx, 1, [&](psize_t block, psize_t row) -> const auto& {
                    return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
                });
            }
            else {
                return make_generic_error("Substream {} is empty", Idx);
            }
        }


        template <typename CtrT, typename NTypes, typename... Args>
        VoidResult treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, Args&&... args) noexcept
        {
            MEMORIA_TRY_VOID(node.layout(255));
            return node.template processSubstreams<IntList<Stream>>(*this, idx, std::forward<Args>(args)...);
        }
    };



    template <int32_t Stream, typename Entry>
    VoidResult ctr_try_insert_stream_entry_no_mgr(const TreeNodePtr& leaf, int32_t idx, const Entry& entry) noexcept
    {
        InsertStreamEntryFn<Stream> fn;
        return self().leaf_dispatcher().dispatch(leaf, fn, idx, entry);
    }


    template <int32_t Stream, typename Entry>
    bool ctr_try_insert_stream_entry(
            Iterator& iter,
            int32_t idx,
            const Entry& entry
    )
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(iter.path(), 0);

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(iter.iter_leaf());

        mgr.add(iter.iter_leaf().as_mutable());

        auto status = self.template ctr_try_insert_stream_entry_no_mgr<Stream>(iter.iter_leaf().as_mutable(), idx, entry);
        if (status.is_error()) {
            if (status.is_packed_error())
            {
                mgr.rollback();
                return false;
            }
            else {
                MEMORIA_PROPAGATE_ERROR(status).do_throw();
            }
        }

        return true;
    }

    bool ctr_with_block_manager(
            TreeNodePtr& leaf,
            int structure_idx,
            int stream_idx,
            const std::function<VoidResult (int, int)>& insert_fn
    )
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(leaf);

        mgr.add(leaf);

        VoidResult status = insert_fn(structure_idx, stream_idx);

        if (status.is_ok()) {
            return true;
        }
        else if (status.memoria_error()->error_category() == ErrorCategory::PACKED)
        {
            mgr.rollback();
            return false;
        }
        else {
            status.get_or_throw();
            return false;
        }
    }





    template <int32_t Stream>
    struct RemoveFromLeafFn
    {
        template <
            int32_t Idx,
            typename SubstreamType
        >
        VoidResult stream(SubstreamType&& obj, int32_t idx) noexcept
        {
            return obj.remove_entries(idx, 1);
        }

        template <typename CtrT, typename NTypes>
        VoidResult treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx) noexcept
        {
            MEMORIA_TRY_VOID(node.layout(255));
            return node.template processSubstreams<IntList<Stream>>(*this, idx);
        }
    };


    template <int32_t Stream>
    bool ctr_try_remove_stream_entry(TreePathT& path, int32_t idx)
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(path, 0);

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(path.leaf());
        mgr.add(path.leaf().as_mutable());

        RemoveFromLeafFn<Stream> fn;
        VoidResult status = self.leaf_dispatcher().dispatch(path.leaf().as_mutable(), fn, idx);

        if (status.is_ok()) {
            return true;
        }
        else if (status.is_packed_error()) {
            mgr.rollback();
            return false;
        }
        else {
            status.get_or_throw();
            return false;
        }
    }




    //=========================================================================================

    template <typename SubstreamsList>
    struct UpdateStreamEntryBufferFn
    {
        template <
            int32_t StreamIdx,
            int32_t AllocatorIdx,
            int32_t Idx,
            typename SubstreamType,
            typename Entry
        >
        VoidResult stream(SubstreamType&& obj, int32_t idx, const Entry& entry) noexcept
        {
            return obj.update_entries(idx, 1, [&](auto block, auto){
                return entry.get(bt::StreamTag<StreamIdx>(), bt::StreamTag<Idx>(), block);
            });
        }

        template <typename CtrT, typename NTypes, typename... Args>
        VoidResult treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, Args&&... args) noexcept
        {
            return node.template processSubstreams<
                SubstreamsList
            >(
                    *this,
                    idx,
                    std::forward<Args>(args)...
            );
        }
    };


    template <typename SubstreamsList, typename Entry>
    bool ctr_try_update_stream_entry(Iterator& iter, int32_t idx, const Entry& entry)
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(iter.path(), 0);

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(iter.iter_leaf().as_mutable());
        mgr.add(iter.iter_leaf().as_mutable());


        UpdateStreamEntryBufferFn<SubstreamsList> fn;
        VoidResult status = self.leaf_dispatcher().dispatch(
                    iter.iter_leaf().as_mutable(),
                    fn,
                    idx,
                    entry
        );

        if (status.is_ok()) {
            return true;
        }
        else if (status.memoria_error()->error_category() == ErrorCategory::PACKED)
        {
            mgr.rollback();
            return false;
        }
        else {
            status.get_or_throw();
            return false;
        }
    }

    template <typename Fn, typename... Args>
    bool update(Iterator& iter, Fn&& fn, Args&&... args)
    {
        auto& self = this->self();
        return self.ctr_update_atomic(iter, std::forward<Fn>(fn), VLSelector(), std::forward<Fn>(args)...);
    }


    MEMORIA_V1_DECLARE_NODE_FN(TryMergeNodesFn, mergeWith);
    bool ctr_try_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path);

    bool ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, bool only_if_same_parent = false);

    bool ctr_merge_current_leaf_nodes(TreePathT& tgt, TreePathT& src);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
bool M_TYPE::ctr_try_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path)
{
    auto& self = this->self();

    self.ctr_check_same_paths(tgt_path, src_path, 1);

    auto src_parent = self.ctr_get_node_parent(src_path, 0);
    auto parent_idx = self.ctr_get_child_idx(src_parent, src_path.leaf()->id());

    self.ctr_cow_clone_path(tgt_path, 0);

    BlockUpdateMgr mgr(self);

    TreeNodeConstPtr tgt = tgt_path.leaf();
    TreeNodeConstPtr src = src_path.leaf();

    self.ctr_update_block_guard(tgt);

    // FIXME: Need to leave src node untouched on merge.
    mgr.add(tgt.as_mutable());

    //MEMORIA_TRY(tgt_sizes, self.ctr_get_node_sizes(tgt));



    VoidResult res = self.leaf_dispatcher().dispatch_1st_const(src, tgt.as_mutable(), TryMergeNodesFn());

    if (res.is_error())
    {
        if (res.is_packed_error())
        {
            mgr.rollback();
            return false;
        }
        else {
            MEMORIA_PROPAGATE_ERROR(res).do_throw();
        }
    }


    self.ctr_remove_non_leaf_node_entry(tgt_path, 1, parent_idx).get_or_throw();

    self.ctr_update_path(tgt_path, 0);

    self.ctr_cow_ref_children_after_merge(src.as_mutable());

    self.ctr_unref_block(src->id());

    self.ctr_check_path(tgt_path, 0);

    return true;
}


M_PARAMS
bool M_TYPE::ctr_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path, bool only_if_same_parent)
{
    auto& self = this->self();

    auto same_parent = self.ctr_is_the_same_parent(tgt_path, src_path, 0);

    if (same_parent)
    {
        auto merged = self.ctr_merge_current_leaf_nodes(tgt_path, src_path);
        if (!merged)
        {
            self.ctr_assign_path_nodes(tgt_path, src_path);
            self.ctr_expect_next_node(src_path, 0);
        }

        return merged;
    }
    else if (!only_if_same_parent)
    {
        auto merged = self.ctr_merge_branch_nodes(tgt_path, src_path, 1, only_if_same_parent);

        self.ctr_assign_path_nodes(tgt_path, src_path, 0);
        self.ctr_expect_next_node(src_path, 0);

        if (merged)
        {
            return self.ctr_merge_current_leaf_nodes(tgt_path, src_path);
        }
    }

    return false;
}




M_PARAMS
bool M_TYPE::ctr_merge_current_leaf_nodes(TreePathT& tgt, TreePathT& src)
{
    auto& self = this->self();

    auto merged = self.ctr_try_merge_leaf_nodes(tgt, src);
    if (merged)
    {
        self.ctr_remove_redundant_root(tgt, 0);
        return true;
    }
    else {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS

}
