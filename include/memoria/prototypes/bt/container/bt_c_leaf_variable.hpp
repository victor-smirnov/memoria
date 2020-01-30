
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
        OpStatus status_{OpStatus::OK};

        template <
            int32_t Offset,
            bool StreamStart,
            int32_t Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem,
            typename Entry
        >
        void stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry)
        {
            if (isOk(status_)) {
                status_ <<= obj.template _insert_b<Offset>(idx, accum, [&](int32_t block) -> const auto& {
                        return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
                });
            }
        }


        template <typename CtrT, typename NTypes, typename... Args>
        void treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, BranchNodeEntry& accum, Args&&... args)
        {
            node.layout(255);
            node.template processStreamAcc<Stream>(*this, accum, idx, std::forward<Args>(args)...);
        }
    };



    template <int32_t Stream, typename Entry>
    OpStatus ctr_try_insert_stream_entry_no_mgr(NodeBaseG& leaf, int32_t idx, const Entry& entry) noexcept
    {
        BranchNodeEntry accum;
        InsertStreamEntryFn<Stream> fn;
        self().leaf_dispatcher().dispatch(leaf, fn, idx, accum, entry);

        return fn.status_;
    }


    template <int32_t Stream, typename Entry>
    Result<std::tuple<bool>> ctr_try_insert_stream_entry(Iterator& iter, int32_t idx, const Entry& entry) noexcept
    {
        using ResultT = Result<std::tuple<bool>>;

        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_block_guard(iter.iter_leaf()));

        MEMORIA_RETURN_IF_ERROR_FN(mgr.add(iter.iter_leaf()));

        auto status = self.template ctr_try_insert_stream_entry_no_mgr<Stream>(iter.iter_leaf(), idx, entry);

        if (isFail(status))
        {
            mgr.rollback();
            return ResultT::of(std::make_tuple(false));
        }

        return ResultT::of(std::make_tuple(true));
    }

    BoolResult ctr_with_block_manager(NodeBaseG& leaf, int structure_idx, int stream_idx, std::function<Result<OpStatus> (int, int)> insert_fn) noexcept
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(leaf);

        MEMORIA_RETURN_IF_ERROR_FN(mgr.add(leaf));

        auto status = insert_fn(structure_idx, stream_idx);
        MEMORIA_RETURN_IF_ERROR(status);

        if (isFail(status.get()))
        {
            mgr.rollback();
            return BoolResult::of(false);
        }

        return BoolResult::of(true);
    }





    template <int32_t Stream>
    struct RemoveFromLeafFn
    {
        OpStatus status_{OpStatus::OK};

        template <
            int32_t Offset,
            bool StreamStart,
            int32_t Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem
        >
        void stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx)
        {
            if (isOk(status_)) {
                status_ <<= obj.template _remove<Offset>(idx, accum);
            }
        }

        template <typename CtrT, typename NTypes>
        void treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, BranchNodeEntry& accum)
        {
            node.layout(255);
            node.template processStreamAcc<Stream>(*this, accum, idx);
        }
    };


    template <int32_t Stream>
    Result<std::tuple<bool, BranchNodeEntry>> ctr_try_remove_stream_entry(Iterator& iter, int32_t idx) noexcept
    {
        using ResultT = Result<std::tuple<bool, BranchNodeEntry>>;
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_block_guard(iter.iter_leaf()));

        MEMORIA_RETURN_IF_ERROR_FN(mgr.add(iter.iter_leaf()));

        BranchNodeEntry accum;
        RemoveFromLeafFn<Stream> fn;
        self.leaf_dispatcher().dispatch(iter.iter_leaf(), fn, idx, accum);

        if (isFail(fn.status_)) {
            mgr.rollback();
            return ResultT::of(std::make_tuple(false, BranchNodeEntry()));
        }

        return ResultT::of(std::make_tuple(true, accum));
    }




    //=========================================================================================

    template <int32_t Stream, typename SubstreamsList>
    struct UpdateStreamEntryBufferFn
    {
        OpStatus status_{OpStatus::OK};
        template <
            int32_t Offset,
            bool Start,
            int32_t Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem,
            typename Entry
        >
        void stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry)
        {
            if (isOk(status_)) {
                status_ <<= obj.template _update_b<Offset>(idx, accum, [&](int32_t block){
                    return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
                });
            }
        }

        template <typename CtrT, typename NTypes, typename... Args>
        void treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, BranchNodeEntry& accum, Args&&... args)
        {
            node.template processSubstreamsByIdxAcc<
                Stream,
                SubstreamsList
            >(
                    *this,
                    accum,
                    idx,
                    std::forward<Args>(args)...
            );
        }
    };


    template <int32_t Stream, typename SubstreamsList, typename Entry>
    Result<std::tuple<bool, BranchNodeEntry>> ctr_try_update_stream_entry(Iterator& iter, int32_t idx, const Entry& entry) noexcept
    {
        using ResultT = Result<std::tuple<bool, BranchNodeEntry>>;
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        auto res = self.ctr_update_block_guard(iter.iter_leaf());
        MEMORIA_RETURN_IF_ERROR(res);

        MEMORIA_RETURN_IF_ERROR_FN(mgr.add(iter.iter_leaf()));

        BranchNodeEntry accum;
        UpdateStreamEntryBufferFn<Stream, SubstreamsList> fn;
        self.leaf_dispatcher().dispatch(
                    iter.iter_leaf(),
                    fn,
                    idx,
                    accum,
                    entry
        );

        if (isFail(fn.status_))
        {
            mgr.rollback();
            return ResultT::of(std::make_tuple(false, BranchNodeEntry()));
        }

        return ResultT::of(std::make_tuple(true, accum));
    }

    template <typename Fn, typename... Args>
    BoolResult update(Iterator& iter, Fn&& fn, Args&&... args) noexcept
    {
        auto& self = this->self();
        return self.ctr_update_atomic(iter, std::forward<Fn>(fn), VLSelector(), std::forward<Fn>(args)...);
    }

    // FIXME: not used
    // NodeBaseG createNextLeaf(NodeBaseG& leaf);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(TryMergeNodesFn, mergeWith, OpStatus);
    BoolResult ctr_try_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path, MergeFn = [](const Position&){
        return VoidResult::of();
    }) noexcept;

    BoolResult ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, MergeFn fn = [](const Position&){
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

    BlockUpdateMgr mgr(self);

    NodeBaseG tgt = tgt_path.leaf();
    NodeBaseG src = src_path.leaf();

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(src));
    MEMORIA_TRY_VOID(self.ctr_update_block_guard(tgt));

    MEMORIA_TRY_VOID(mgr.add(src));
    MEMORIA_TRY_VOID(mgr.add(tgt));

    Position tgt_sizes = self.ctr_get_node_sizes(tgt);
    //int32_t tgt_size   = self.ctr_get_node_size(tgt, 0);

    Result<NodeBaseG> src_parent = self.ctr_get_node_parent(src_path, 0);
    MEMORIA_RETURN_IF_ERROR(src_parent);

    MEMORIA_TRY(parent_idx, self.ctr_get_child_idx(src_parent.get(), src->id()));

    if (isFail(self.leaf_dispatcher().dispatch(src, tgt, TryMergeNodesFn()))) {
        mgr.rollback();
        return BoolResult::of(false);
    }

    BranchNodeEntry max = self.ctr_get_node_max_keys(tgt);

    // FIXME. This is special OOM condition that, if occurs, must be handled separately.
    Result<OpStatus> status1 = self.ctr_remove_non_leaf_node_entry(src_path, 1, parent_idx);
    MEMORIA_RETURN_IF_ERROR(status1);

    if (isFail(status1.get())) {
        return BoolResult::make_error("PackedOOMException");
    }

    int32_t idx = parent_idx - 1;

    MEMORIA_TRY_VOID(self.ctr_update_branch_nodes(src_path, 1, idx, max));

    MEMORIA_RETURN_IF_ERROR_FN(self.store().removeBlock(src->id()));

    MEMORIA_RETURN_IF_ERROR_FN(fn(tgt_sizes));

    return BoolResult::of(true);
}


M_PARAMS
BoolResult M_TYPE::ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, MergeFn fn) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY(same_parent, self.ctr_is_the_same_parent(tgt, src, 0));

    if (same_parent)
    {
        return self.ctr_merge_current_leaf_nodes(tgt, src, fn);
    }
    else
    {
        auto res = self.ctr_merge_branch_nodes(tgt, src, 1);
        MEMORIA_RETURN_IF_ERROR(res);

        if (res.get())
        {
            return self.ctr_merge_current_leaf_nodes(tgt, src, fn);
        }
        else
        {
            return BoolResult::of(false);
        }
    }
}




M_PARAMS
BoolResult M_TYPE::ctr_merge_current_leaf_nodes(TreePathT& tgt, TreePathT& src, MergeFn fn) noexcept
{
    auto& self = this->self();

    auto res0 = self.ctr_try_merge_leaf_nodes(tgt, src, fn);
    MEMORIA_RETURN_IF_ERROR(res0);
    if (res0.get())
    {
        auto res1 = self.ctr_remove_redundant_root(tgt, 0);
        MEMORIA_RETURN_IF_ERROR(res1);

        return BoolResult::of(true);
    }
    else {
        return BoolResult::of(false);
    }
}


#undef M_TYPE
#undef M_PARAMS

}
