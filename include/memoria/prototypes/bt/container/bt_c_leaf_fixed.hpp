
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

#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>
#include <memoria/core/tools/result.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::LeafFixedName)

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::Iterator;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::BranchNodeEntry;

    template <
        int32_t Stream
    >
    struct InsertStreamEntryFn
    {
        template <
            int32_t Idx,
            typename SubstreamType,
            typename Entry
        >
        VoidResult stream(SubstreamType&& obj, int32_t idx, const Entry& entry) noexcept
        {
            if (obj) {
                return obj.insert_entries(idx, 1, [&](int32_t block, int32_t row) -> const auto& {
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
    bool ctr_try_insert_stream_entry(Iterator& iter, int32_t idx, const Entry& entry)
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(iter.path(), 0);

        auto capacity = self.ctr_check_node_capacities(iter.iter_leaf(), Position::create(Stream, 1));
        if (capacity)
        {
            self.ctr_update_block_guard(iter.iter_leaf());
            self.leaf_dispatcher().dispatch(iter.iter_leaf().as_mutable(), InsertStreamEntryFn<Stream>(), idx, entry).get_or_throw();
            return true;
        }
        else {
            return false;
        }
    }








    template <int32_t Stream>
    struct RemoveFromLeafFn
    {
        template <typename CtrT, typename NTypes>
        VoidResult treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx) noexcept
        {
            MEMORIA_TRY_VOID(node.layout(255));
            return node.template processSubstreams<IntList<Stream>>(*this, idx);
        }

        template <
            int32_t Idx,
            typename SubstreamType
        >
        VoidResult stream(SubstreamType&& obj, int32_t idx) noexcept
        {
            return obj.remove_entries(idx, 1);
        }
    };

    template <int32_t Stream>
    bool ctr_try_remove_stream_entry(TreePathT& path, int32_t idx)
    {
        self().ctr_update_block_guard(path.leaf());

        self().leaf_dispatcher().dispatch(path.leaf().as_mutable(), RemoveFromLeafFn<Stream>(), idx).get_or_throw();

        return true;
    }




    //=========================================================================================


    template <typename SubstreamsList>
    struct UpdateStreamEntryFn
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
            return obj.update_entries(idx, 1, [&](int32_t block, int32_t row) noexcept {
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

        self.ctr_update_block_guard(iter.iter_leaf());

        self.leaf_dispatcher().dispatch(
                iter.iter_leaf().as_mutable(),
                UpdateStreamEntryFn<SubstreamsList>(),
                idx,
                entry
        ).get_or_throw();

        return true;
    }





    //==========================================================================================

    MEMORIA_V1_DECLARE_NODE2_FN(CanMergeFn, canBeMergedWith);
    bool ctr_can_merge_nodes(const TreeNodeConstPtr& tgt, const TreeNodeConstPtr& src)
    {
        return self().node_dispatcher().dispatch(src, tgt, CanMergeFn()).get_or_throw();
    }


    MEMORIA_V1_DECLARE_NODE_FN(MergeNodesFn, mergeWith);
    void ctr_do_merge_leaf_nodes(TreePathT& tgt, TreePathT& src);

    bool ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, bool only_if_same_parent = false);

    bool ctr_merge_current_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::ctr_do_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path)
{
    auto& self = this->self();

    self.ctr_check_same_paths(tgt_path, src_path, 1);

    self.ctr_cow_clone_path(tgt_path, 0);

    TreeNodeConstPtr src = src_path.leaf();
    TreeNodeConstPtr tgt = tgt_path.leaf();

    self.ctr_update_block_guard(tgt);

    self.leaf_dispatcher().dispatch_1st_const(src, tgt.as_mutable(), MergeNodesFn()).get_or_throw();

    auto parent_idx = self.ctr_get_parent_idx(src_path, 0);

    self.ctr_remove_non_leaf_node_entry(tgt_path, 1, parent_idx).get_or_throw();

    self.ctr_update_path(tgt_path, 0);

    self.ctr_check_path(tgt_path, 0);

    self.ctr_cow_ref_children_after_merge(src.as_mutable());
    return self.ctr_unref_block(src->id());
}


M_PARAMS
bool M_TYPE::ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, bool only_if_same_parent)
{
    auto& self = this->self();

    auto can_be_merged = self.ctr_can_merge_nodes(tgt.leaf(), src.leaf());
    if (can_be_merged)
    {
        auto is_same_parent = self.ctr_is_the_same_parent(tgt, src, 0);

        if (is_same_parent)
        {
            self.ctr_do_merge_leaf_nodes(tgt, src);
            self.ctr_remove_redundant_root(tgt, 0);

            return true;
        }
        else if (!only_if_same_parent)
        {
            auto branch_nodes_merged = self.ctr_merge_branch_nodes(tgt, src, 1);

            self.ctr_assign_path_nodes(tgt, src, 0);
            self.ctr_expect_next_node(src, 0);

            if (branch_nodes_merged)
            {
                self.ctr_do_merge_leaf_nodes(tgt, src);
                self.ctr_remove_redundant_root(tgt, 0);

                return true;
            }
            return false;
        }
    }

    return false;
}



M_PARAMS
bool M_TYPE::ctr_merge_current_leaf_nodes(
        TreePathT& tgt_path,
        TreePathT& src_path
)
{
    auto& self = this->self();

    TreeNodeConstPtr src = src_path.leaf();
    TreeNodeConstPtr tgt = tgt_path.leaf();

    if (self.ctr_can_merge_nodes(tgt, src))
    {
        fn(self.ctr_get_node_sizes(tgt));

        self.ctr_do_merge_leaf_nodes(tgt_path, src_path);
        self.ctr_remove_redundant_root(tgt_path);

        return true;
    }
    else {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS

}
