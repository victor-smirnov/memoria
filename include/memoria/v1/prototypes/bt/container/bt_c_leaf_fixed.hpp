
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

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

    static const int32_t Streams = Types::Streams;

    // TODO: noexcept
    template <
        int32_t Stream
    >
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
        void stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry)
        {
            OOM_THROW_IF_FAILED(obj.template _insert_b<Offset>(idx, accum, [&](int32_t block) -> const auto& {
                return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
            }), MMA1_SRC);
        }

        template <typename CtrT, typename NTypes, typename... Args>
        void treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, BranchNodeEntry& accum, Args&&... args)
        {
            node.layout(255);
            node.template processStreamAcc<Stream>(*this, accum, idx, std::forward<Args>(args)...);
        }
    };


    template <int32_t Stream, typename Entry>
    Result<std::tuple<bool>> ctr_try_insert_stream_entry(Iterator& iter, int32_t idx, const Entry& entry) noexcept
    {
        using ResultT = Result<std::tuple<bool>>;
        auto& self = this->self();

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_block_guard(iter.iter_leaf()));

        if (self.ctr_check_node_capacities(iter.iter_leaf(), Position::create(Stream, 1)))
        {
            BranchNodeEntry accum;
            self.leaf_dispatcher().dispatch(iter.iter_leaf(), InsertStreamEntryFn<Stream>(), idx, accum, entry);            
            return ResultT::of(std::make_tuple(true));
        }
        else {
            return ResultT::of(std::make_tuple(false));
        }
    }








    template <int32_t Stream>
    struct RemoveFromLeafFn
    {
        template <typename CtrT, typename NTypes>
        void treeNode(LeafNodeSO<CtrT, NTypes>& node, int32_t idx, BranchNodeEntry& accum)
        {
            node.layout(255);
            node.template processStreamAcc<Stream>(*this, accum, idx);
        }

        template <
            int32_t Offset,
            bool StreamStart,
            int32_t Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem
        >
        void stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx)
        {
            OOM_THROW_IF_FAILED(obj.template _remove<Offset>(idx, accum), MMA1_SRC);
        }
    };

    template <int32_t Stream>
    Result<std::tuple<bool, BranchNodeEntry>> ctr_try_remove_stream_entry(Iterator& iter, int32_t idx) noexcept
    {
        using ResultT = Result<std::tuple<bool, BranchNodeEntry>>;
        MEMORIA_RETURN_IF_ERROR_FN(self().ctr_update_block_guard(iter.iter_leaf()));

        BranchNodeEntry accum;
        self().leaf_dispatcher().dispatch(iter.iter_leaf(), RemoveFromLeafFn<Stream>(), idx, accum);
        return ResultT::of(std::make_tuple(true, accum));
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
        void stream(SubstreamType&& obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry)
        {
            OOM_THROW_IF_FAILED(obj.template _update_b<Offset>(idx, accum, [&](int32_t block) {
                return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
            }), MMA1_SRC);
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

        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_block_guard(iter.iter_leaf()));

        BranchNodeEntry accum;
        self.leaf_dispatcher().dispatch(
                iter.iter_leaf(),
                UpdateStreamEntryFn<Stream, SubstreamsList>(),
                idx,
                accum,
                entry
        );

        return ResultT::of(std::make_tuple(true, accum));
    }





    //==========================================================================================

    MEMORIA_V1_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);
    bool ctr_can_merge_nodes(const NodeBaseG& tgt, const NodeBaseG& src) noexcept
    {
        return self().node_dispatcher().dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(MergeNodesFn, mergeWith, OpStatus);
    VoidResult ctr_do_merge_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src) noexcept;

    BoolResult ctr_merge_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){
        return VoidResult::of();
    }) noexcept;

    BoolResult ctr_merge_current_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){
        return VoidResult::of();
    }) noexcept;

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
VoidResult M_TYPE::ctr_do_merge_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src) noexcept
{
    auto& self = this->self();

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_block_guard(tgt));
    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_block_guard(src));

    int32_t tgt_size = self.ctr_get_node_size(tgt, 0);

    OpStatus status1 = self.leaf_dispatcher().dispatch(src, tgt, MergeNodesFn());
    if (isFail(status1)) {
        return VoidResult::make_error("PackedOOMException");
    }

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_children(tgt, tgt_size));

    Result<NodeBaseG> src_parent = self.ctr_get_node_parent(src);
    MEMORIA_RETURN_IF_ERROR(src_parent);

    int32_t parent_idx = src->parent_idx();

    //MEMORIA_V1_ASSERT(parent_idx, >, 0);

    Result<OpStatus> status2 = self.ctr_remove_non_leaf_node_entry(src_parent.get(), parent_idx);
    MEMORIA_RETURN_IF_ERROR(status2);

    if (isFail(status2.get())) {
        return VoidResult::make_error("PackedOOMException");
    }

    int32_t idx = parent_idx - 1;

    auto max = self.ctr_get_node_max_keys(tgt);

    MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_branch_nodes(src_parent.get(), idx, max));

    return self.store().removeBlock(src->id());
}


M_PARAMS
BoolResult M_TYPE::ctr_merge_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn) noexcept
{
    auto& self = this->self();

    if (self.ctr_can_merge_nodes(tgt, src))
    {
        if (self.ctr_is_the_same_parent(tgt, src))
        {
            auto sizes = self.ctr_get_node_sizes(tgt);

            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_do_merge_leaf_nodes(tgt, src));
            MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_redundant_root(tgt));

            auto res = fn(sizes);
            MEMORIA_RETURN_IF_ERROR(res);

            return BoolResult::of(true);
        }
        else
        {
            Result<NodeBaseG> tgt_parent = self.ctr_get_node_parent(tgt);
            MEMORIA_RETURN_IF_ERROR(tgt_parent);

            Result<NodeBaseG> src_parent = self.ctr_get_node_parent(src);
            MEMORIA_RETURN_IF_ERROR(src_parent);

            auto res0 = self.ctr_merge_branch_nodes(tgt_parent.get(), src_parent.get());
            MEMORIA_RETURN_IF_ERROR(res0);

            if (res0.get())
            {
                auto sizes = self.ctr_get_node_sizes(tgt);

                MEMORIA_RETURN_IF_ERROR_FN(self.ctr_do_merge_leaf_nodes(tgt, src));

                MEMORIA_RETURN_IF_ERROR_FN(self.ctr_remove_redundant_root(tgt));

                auto res1 = fn(sizes);
                MEMORIA_RETURN_IF_ERROR(res1);

                return BoolResult::of(true);
            }
            else
            {
                return BoolResult::of(false);
            }
        }
    }
    else
    {
        return BoolResult::of(false);
    }
}



M_PARAMS
BoolResult M_TYPE::ctr_merge_current_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn) noexcept
{
    auto& self = this->self();

    if (self.ctr_can_merge_nodes(tgt, src))
    {
        auto res0 = fn(self.ctr_get_node_sizes(tgt));
        MEMORIA_RETURN_IF_ERROR(res0);

        auto res1 = self.ctr_do_merge_leaf_nodes(tgt, src);
        MEMORIA_RETURN_IF_ERROR(res1);

        auto res2 = self.ctr_remove_redundant_root(tgt);
        MEMORIA_RETURN_IF_ERROR(res2);

        return BoolResult::of(true);
    }
    else
    {
        return BoolResult::of(false);
    }
}


#undef M_TYPE
#undef M_PARAMS

}}
