
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
#include <memoria/core/tools/result.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::LeafFixedName)
public:
    using Types = typename Base::Types;

public:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

    typedef std::function<VoidResult (const Position&)>                         MergeFn;

    using CtrSizeT = typename Types::CtrSizeT;

    using typename Base::TreePathT;

    static const int32_t Streams = Types::Streams;

    // TODO: noexcept
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
    BoolResult ctr_try_insert_stream_entry(Iterator& iter, int32_t idx, const Entry& entry) noexcept
    {
        using ResultT = BoolResult;
        auto& self = this->self();

        MEMORIA_TRY(capacity, self.ctr_check_node_capacities(iter.iter_leaf(), Position::create(Stream, 1)));
        if (capacity)
        {
            MEMORIA_TRY_VOID(self.ctr_update_block_guard(iter.iter_leaf()));
            MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch(iter.iter_leaf(), InsertStreamEntryFn<Stream>(), idx, entry));
            return ResultT::of(true);
        }
        else {
            return ResultT::of(false);
        }
    }








    template <int32_t Stream>
    struct RemoveFromLeafFn
    {
        template <typename CtrT, typename NTypes>
        VoidResult treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx) noexcept
        {
            MEMORIA_TRY_VOID(node.layout(255));
            MEMORIA_TRY_VOID(node.template processSubstreams<IntList<Stream>>(*this, idx));
            return VoidResult::of();
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
    BoolResult ctr_try_remove_stream_entry(Iterator& iter, int32_t idx) noexcept
    {
        using ResultT = BoolResult;
        MEMORIA_TRY_VOID(self().ctr_update_block_guard(iter.iter_leaf()));

        MEMORIA_TRY_VOID(self().leaf_dispatcher().dispatch(iter.iter_leaf(), RemoveFromLeafFn<Stream>(), idx));

        return ResultT::of(true);
    }




    //=========================================================================================


    template <int32_t Stream, typename SubstreamsList>
    struct UpdateStreamEntryFn
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
            return obj.template _update_b<Offset>(idx, accum, [&](int32_t block) {
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

        MEMORIA_TRY_VOID(self.ctr_update_block_guard(iter.iter_leaf()));

        BranchNodeEntry accum;
        MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch(
                iter.iter_leaf(),
                UpdateStreamEntryFn<Stream, SubstreamsList>(),
                idx,
                accum,
                entry
        ));

        return ResultT::of(std::make_tuple(true, std::move(accum)));
    }





    //==========================================================================================

    MEMORIA_V1_DECLARE_NODE2_FN(CanMergeFn, canBeMergedWith);
    bool ctr_can_merge_nodes(const NodeBaseG& tgt, const NodeBaseG& src) noexcept
    {
        return self().node_dispatcher().dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(MergeNodesFn, mergeWith);
    VoidResult ctr_do_merge_leaf_nodes(TreePathT& tgt, TreePathT& src) noexcept;

    BoolResult ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, bool only_if_same_parent = false, MergeFn fn = [](const Position&){
        return VoidResult::of();
    }) noexcept;

    BoolResult ctr_merge_current_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path, MergeFn fn = [](const Position&){
        return VoidResult::of();
    }) noexcept;

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
VoidResult M_TYPE::ctr_do_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY_VOID(self.ctr_check_same_paths(tgt_path, src_path, 1));

    NodeBaseG src = src_path.leaf();
    NodeBaseG tgt = tgt_path.leaf();

    MEMORIA_TRY_VOID(self.ctr_update_block_guard(tgt));

    MEMORIA_TRY_VOID(self.leaf_dispatcher().dispatch_1st_const(src, tgt, MergeNodesFn()));

    MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(src_path, 0));

    MEMORIA_TRY_VOID(self.ctr_remove_non_leaf_node_entry(tgt_path, 1, parent_idx));

    MEMORIA_TRY_VOID(self.ctr_update_path(tgt_path, 0));

    MEMORIA_TRY_VOID(self.ctr_check_path(tgt_path, 0));

    return self.store().removeBlock(src->id());
}


M_PARAMS
BoolResult M_TYPE::ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, bool only_if_same_parent, MergeFn fn) noexcept
{
    auto& self = this->self();

    MEMORIA_TRY(can_be_merged, self.ctr_can_merge_nodes(tgt.leaf(), src.leaf()));
    if (can_be_merged)
    {
        MEMORIA_TRY(is_same_parent, self.ctr_is_the_same_parent(tgt, src, 0));

        if (is_same_parent)
        {
            MEMORIA_TRY(sizes, self.ctr_get_node_sizes(tgt.leaf()));

            MEMORIA_TRY_VOID(self.ctr_do_merge_leaf_nodes(tgt, src));
            MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(tgt));

            MEMORIA_TRY_VOID(fn(sizes));

            return BoolResult::of(true);
        }
        else if (!only_if_same_parent)
        {
            MEMORIA_TRY(branch_nodes_merged, self.ctr_merge_branch_nodes(tgt, src, 1));

            MEMORIA_TRY_VOID(self.ctr_assign_path_nodes(tgt, src, 0));
            MEMORIA_TRY_VOID(self.ctr_expect_next_node(src, 0));

            if (branch_nodes_merged)
            {
                MEMORIA_TRY(sizes, self.ctr_get_node_sizes(tgt.leaf()));

                MEMORIA_TRY_VOID(self.ctr_do_merge_leaf_nodes(tgt, src));
                MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(tgt));

                MEMORIA_TRY_VOID(fn(sizes));

                return BoolResult::of(true);
            }
            return BoolResult::of(false);
        }
    }

    return BoolResult::of(false);
}



M_PARAMS
BoolResult M_TYPE::ctr_merge_current_leaf_nodes(
        TreePathT& tgt_path,
        TreePathT& src_path,
        MergeFn fn
) noexcept
{
    auto& self = this->self();

    NodeBaseG src = src_path.leaf();
    NodeBaseG tgt = tgt_path.leaf();

    if (self.ctr_can_merge_nodes(tgt, src))
    {
        MEMORIA_TRY_VOID(
            fn(self.ctr_get_node_sizes(tgt))
        );

        MEMORIA_TRY_VOID(self.ctr_do_merge_leaf_nodes(tgt_path, src_path));
        MEMORIA_TRY_VOID(self.ctr_remove_redundant_root(tgt_path));

        return BoolResult::of(true);
    }
    else
    {
        return BoolResult::of(false);
    }
}


#undef M_TYPE
#undef M_PARAMS

}
