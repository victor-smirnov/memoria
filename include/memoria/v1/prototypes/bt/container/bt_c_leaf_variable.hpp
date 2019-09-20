
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

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;

    typedef std::function<void (const Position&)>                               MergeFn;

    static const int32_t Streams                                                    = Types::Streams;
public:
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
    OpStatus ctr_try_insert_stream_entry_no_mgr(NodeBaseG& leaf, int32_t idx, const Entry& entry)
    {
        BranchNodeEntry accum;
        InsertStreamEntryFn<Stream> fn;
        self().leaf_dispatcher().dispatch(leaf, fn, idx, accum, entry);

        return fn.status_;
    }


    template <int32_t Stream, typename Entry>
    MMA1_NODISCARD std::tuple<bool> ctr_try_insert_stream_entry(Iterator& iter, int32_t idx, const Entry& entry)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(iter.iter_leaf());

        mgr.add(iter.iter_leaf());

        auto status = self.template ctr_try_insert_stream_entry_no_mgr<Stream>(iter.iter_leaf(), idx, entry);

        if (isFail(status))
        {
            mgr.rollback();
            return std::make_tuple(false);
        }

        return std::make_tuple(true);
    }

    MMA1_NODISCARD bool ctr_with_block_manager(NodeBaseG& leaf, int structure_idx, int stream_idx, std::function<OpStatus(int, int)> insert_fn)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(leaf);

        mgr.add(leaf);

        auto status = insert_fn(structure_idx, stream_idx);

        if (isFail(status)) {
            mgr.rollback();
            return false;
        }

        return true;
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
    MMA1_NODISCARD std::tuple<bool, BranchNodeEntry> ctr_try_remove_stream_entry(Iterator& iter, int32_t idx)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(iter.iter_leaf());

        mgr.add(iter.iter_leaf());

        BranchNodeEntry accum;
        RemoveFromLeafFn<Stream> fn;
        self.leaf_dispatcher().dispatch(iter.iter_leaf(), fn, idx, accum);

        if (isFail(fn.status_)) {
            mgr.rollback();
            return std::make_tuple(false, BranchNodeEntry());
        }

        return std::make_tuple(true, accum);
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
    MMA1_NODISCARD std::tuple<bool, BranchNodeEntry> ctr_try_update_stream_entry(Iterator& iter, int32_t idx, const Entry& entry)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(iter.iter_leaf());

        mgr.add(iter.iter_leaf());

        BranchNodeEntry accum;
        UpdateStreamEntryBufferFn<Stream, SubstreamsList> fn;
        self.leaf_dispatcher().dispatch(
                    iter.iter_leaf(),
                    fn,
                    idx,
                    accum,
                    entry
        );

        if (isFail(fn.status_)) {
            mgr.rollback();
            return std::make_tuple(false, BranchNodeEntry());
        }

        return std::make_tuple(true, accum);
    }

    template <typename Fn, typename... Args>
    bool update(Iterator& iter, Fn&& fn, Args&&... args)
    {
        auto& self = this->self();
        return self.ctr_update_atomic(iter, std::forward<Fn>(fn), VLSelector(), std::forward<Fn>(args)...);
    }

    // FIXME: not used
    // NodeBaseG createNextLeaf(NodeBaseG& leaf);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(TryMergeNodesFn, mergeWith, OpStatus);
    MMA1_NODISCARD bool ctr_try_merge_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn = [](const Position&){});
    bool ctr_merge_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});
    bool ctr_merge_current_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
bool M_TYPE::ctr_try_merge_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    BlockUpdateMgr mgr(self);

    self.ctr_update_block_guard(src);
    self.ctr_update_block_guard(tgt);

    mgr.add(src);
    mgr.add(tgt);

    Position tgt_sizes  = self.ctr_get_node_sizes(tgt);

    int32_t tgt_size            = self.ctr_get_node_size(tgt, 0);
    NodeBaseG src_parent        = self.ctr_get_node_parent(src);
    int32_t parent_idx          = src->parent_idx();

    MEMORIA_V1_ASSERT(parent_idx, >, 0);

    if (isFail(self.leaf_dispatcher().dispatch(src, tgt, TryMergeNodesFn()))) {
        mgr.rollback();
        return false;
    }

    self.ctr_update_children(tgt, tgt_size);

    BranchNodeEntry max = self.ctr_get_node_max_keys(tgt);

    // FIXME. This is apecial OOM condition that, if occurs, must be handled separately.
    OOM_THROW_IF_FAILED(self.ctr_remove_non_leaf_node_entry(src_parent, parent_idx), MMA1_SRC);

    int32_t idx = parent_idx - 1;

    self.ctr_update_branch_nodes(src_parent, idx, max);

    self.store().removeBlock(src->id());

    fn(tgt_sizes);

    return true;
}


M_PARAMS
bool M_TYPE::ctr_merge_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    if (self.ctr_is_the_same_parent(tgt, src))
    {
        return self.ctr_merge_current_leaf_nodes(tgt, src, fn);
    }
    else
    {
        NodeBaseG tgt_parent = self.ctr_get_node_parent(tgt);
        NodeBaseG src_parent = self.ctr_get_node_parent(src);

        if (self.ctr_merge_branch_nodes(tgt_parent, src_parent))
        {
            return self.ctr_merge_current_leaf_nodes(tgt, src, fn);
        }
        else
        {
            return false;
        }
    }
}




M_PARAMS
bool M_TYPE::ctr_merge_current_leaf_nodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    if (self.ctr_try_merge_leaf_nodes(tgt, src, fn))
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
