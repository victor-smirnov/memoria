
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
public:
    using Types = typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

    typedef std::function<VoidResult (const Position&)>                         MergeFn;

    using typename Base::TreePathT;

    static const int32_t Streams = Types::Streams;


public:

    // TODO: noexcept
    template <int32_t Stream>
    struct InsertStreamEntryFn
    {
        template <
            int32_t Offset,
            bool StreamStart,
            int32_t Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem,
            typename Entry
        >
        VoidResult stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry) noexcept
        {
            return obj.template _insert_b<Offset>(idx, accum, [&](int32_t block) -> const auto& {
                return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
            });
        }


        template <typename CtrT, typename NTypes, typename... Args>
        VoidResult treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, BranchNodeEntry& accum, Args&&... args) noexcept
        {
            MEMORIA_TRY_VOID(node.layout(255));
            auto res = node.template processStreamAcc<Stream>(*this, accum, idx, std::forward<Args>(args)...);
            MEMORIA_RETURN_IF_ERROR(res);
            return VoidResult::of();
        }
    };



    template <int32_t Stream, typename Entry>
    VoidResult ctr_try_insert_stream_entry_no_mgr(NodeBaseG& leaf, int32_t idx, const Entry& entry) noexcept
    {
        BranchNodeEntry accum;
        InsertStreamEntryFn<Stream> fn;
        return self().leaf_dispatcher().dispatch(leaf, fn, idx, accum, entry);
    }


    template <int32_t Stream, typename Entry>
    Result<bool> ctr_try_insert_stream_entry(Iterator& iter, int32_t idx, const Entry& entry) noexcept
    {
        using ResultT = Result<bool>;

        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        MEMORIA_TRY_VOID(self.ctr_update_block_guard(iter.iter_leaf()));

        MEMORIA_TRY_VOID(mgr.add(iter.iter_leaf()));

        auto status = self.template ctr_try_insert_stream_entry_no_mgr<Stream>(iter.iter_leaf(), idx, entry);
        if (status.is_error()) {
            if (status.is_packed_error())
            {
                mgr.rollback();
                return BoolResult::of(false);
            }
            else {
                return MEMORIA_PROPAGATE_ERROR(status);
            }
        }

        return ResultT::of(true);
    }

    BoolResult ctr_with_block_manager(NodeBaseG& leaf, int structure_idx, int stream_idx, std::function<VoidResult (int, int)> insert_fn) noexcept
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(leaf);

        MEMORIA_TRY_VOID(mgr.add(leaf));

        VoidResult status = insert_fn(structure_idx, stream_idx);

        if (status.is_ok()) {
            return BoolResult::of(true);
        }
        else if (status.memoria_error()->error_category() == ErrorCategory::PACKED)
        {
            mgr.rollback();
            return BoolResult::of(false);
        }
        else {
            return std::move(status).transfer_error();
        }
    }





    template <int32_t Stream>
    struct RemoveFromLeafFn
    {
        template <
            int32_t Offset,
            bool StreamStart,
            int32_t Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem
        >
        VoidResult stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx) noexcept
        {
            return obj.template _remove<Offset>(idx, accum);
        }

        template <typename CtrT, typename NTypes>
        VoidResult treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, BranchNodeEntry& accum) noexcept
        {
            MEMORIA_TRY_VOID(node.layout(255));
            auto res = node.template processStreamAcc<Stream>(*this, accum, idx);
            MEMORIA_RETURN_IF_ERROR(res);
            return VoidResult::of();
        }
    };


    template <int32_t Stream>
    Result<std::tuple<bool, BranchNodeEntry>> ctr_try_remove_stream_entry(Iterator& iter, int32_t idx) noexcept
    {
        using ResultT = Result<std::tuple<bool, BranchNodeEntry>>;
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        MEMORIA_TRY_VOID(self.ctr_update_block_guard(iter.iter_leaf()));
        MEMORIA_TRY_VOID(mgr.add(iter.iter_leaf()));

        BranchNodeEntry accum;
        RemoveFromLeafFn<Stream> fn;
        VoidResult status = self.leaf_dispatcher().dispatch(iter.iter_leaf(), fn, idx, accum);

        if (status.is_ok()) {
            return ResultT::of(std::make_tuple(true, accum));
        }
        else if (status.memoria_error()->error_category() == ErrorCategory::PACKED) {
            mgr.rollback();
            return ResultT::of(std::make_tuple(false, BranchNodeEntry()));
        }
        else {
            return std::move(status).transfer_error();
        }
    }




    //=========================================================================================

    template <int32_t Stream, typename SubstreamsList>
    struct UpdateStreamEntryBufferFn
    {
        template <
            int32_t Offset,
            bool Start,
            int32_t Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem,
            typename Entry
        >
        VoidResult stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry) noexcept
        {
            return obj.template _update_b<Offset>(idx, accum, [&](int32_t block){
                return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
            });
        }

        template <typename CtrT, typename NTypes, typename... Args>
        VoidResult treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, BranchNodeEntry& accum, Args&&... args) noexcept
        {
            auto res = node.template processSubstreamsByIdxAcc<
                Stream,
                SubstreamsList
            >(
                    *this,
                    accum,
                    idx,
                    std::forward<Args>(args)...
            );
            MEMORIA_RETURN_IF_ERROR(res);

            return VoidResult::of();
        }
    };


    template <int32_t Stream, typename SubstreamsList, typename Entry>
    Result<std::tuple<bool, BranchNodeEntry>> ctr_try_update_stream_entry(Iterator& iter, int32_t idx, const Entry& entry) noexcept
    {
        using ResultT = Result<std::tuple<bool, BranchNodeEntry>>;
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        MEMORIA_TRY_VOID(self.ctr_update_block_guard(iter.iter_leaf()));
        MEMORIA_TRY_VOID(mgr.add(iter.iter_leaf()));

        BranchNodeEntry accum;
        UpdateStreamEntryBufferFn<Stream, SubstreamsList> fn;
        VoidResult status = self.leaf_dispatcher().dispatch(
                    iter.iter_leaf(),
                    fn,
                    idx,
                    accum,
                    entry
        );

        if (status.is_ok()) {
            return ResultT::of(std::make_tuple(true, accum));
        }
        else if (status.memoria_error()->error_category() == ErrorCategory::PACKED)
        {
            mgr.rollback();
            return ResultT::of(std::make_tuple(false, BranchNodeEntry()));
        }
        else {
            return std::move(status).transfer_error();
        }
    }

    template <typename Fn, typename... Args>
    BoolResult update(Iterator& iter, Fn&& fn, Args&&... args) noexcept
    {
        auto& self = this->self();
        return self.ctr_update_atomic(iter, std::forward<Fn>(fn), VLSelector(), std::forward<Fn>(args)...);
    }


    MEMORIA_V1_DECLARE_NODE_FN(TryMergeNodesFn, mergeWith);
    BoolResult ctr_try_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path, MergeFn = [](const Position&){
        return VoidResult::of();
    }) noexcept;

    BoolResult ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, bool only_if_same_parent = false, MergeFn fn = [](const Position&){
        return VoidResult::of();
    }) noexcept;

    BoolResult ctr_merge_current_leaf_nodes(TreePathT& tgt, TreePathT& src, MergeFn fn = [](const Position&){
        return VoidResult::of();
    }) noexcept;

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
BoolResult M_TYPE::ctr_try_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path, MergeFn fn) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_check_same_paths(tgt_path, src_path, 1));

    BlockUpdateMgr mgr(self);

    NodeBaseG tgt = tgt_path.leaf();
    NodeBaseG src = src_path.leaf();

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(src));
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(tgt));

    // FIXME: Need to leave src node untouched on merge.
    MEMORIA_TRY_VOID(mgr.add(src));
    MEMORIA_TRY_VOID(mgr.add(tgt));

    Position tgt_sizes = self.ctr_get_node_sizes(tgt);

    MEMORIA_TRY(src_parent, self.ctr_get_node_parent(src_path, 0));
    MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(src_parent, src->id()));

    VoidResult res = self.leaf_dispatcher().dispatch(src, tgt, TryMergeNodesFn());

    if (res.is_error())
    {
        if (res.is_packed_error())
        {
            mgr.rollback();
            return BoolResult::of(false);
        }
        else {
            return MEMORIA_PROPAGATE_ERROR(res);
        }
    }


    MEMORIA_TRY_VOID(self.ctr_remove_non_leaf_node_entry(tgt_path, 1, parent_idx));

    MEMORIA_TRY_VOID(self.ctr_update_path(tgt_path, 0));

    MEMORIA_TRY_VOID(self.store().removeBlock(src->id()));

    MEMORIA_TRY_VOID(fn(tgt_sizes));

    MEMORIA_TRY_VOID(self.ctr_check_path(tgt_path, 0));

    return BoolResult::of(true);
}


M_PARAMS
BoolResult M_TYPE::ctr_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path, bool only_if_same_parent, MergeFn fn) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY(same_parent, self.ctr_is_the_same_parent(tgt_path, src_path, 0));

    if (same_parent)
    {
        MEMORIA_TRY(merged, self.ctr_merge_current_leaf_nodes(tgt_path, src_path, fn));
        if (!merged)
        {
            MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(tgt_path, src_path));
            MEMORIA_TRY_VOID(self.ctr_expect_next_node(src_path, 0));
        }

        return merged_result;
    }
    else if (!only_if_same_parent)
    {
        MEMORIA_TRY(merged, self.ctr_merge_branch_nodes(tgt_path, src_path, 1, only_if_same_parent));

        MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(tgt_path, src_path, 0));
        MEMORIA_TRY_VOID(self.ctr_expect_next_node(src_path, 0));

        if (merged)
        {
            return self.ctr_merge_current_leaf_nodes(tgt_path, src_path, fn);
        }
    }

    return BoolResult::of(false);
}




M_PARAMS
BoolResult M_TYPE::ctr_merge_current_leaf_nodes(TreePathT& tgt, TreePathT& src, MergeFn fn) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY(merged, self.ctr_try_merge_leaf_nodes(tgt, src, fn));
    if (merged)
    {
        MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(tgt, 0));
        return BoolResult::of(true);
    }
    else {
        return BoolResult::of(false);
    }
}


#undef M_TYPE
#undef M_PARAMS

}
