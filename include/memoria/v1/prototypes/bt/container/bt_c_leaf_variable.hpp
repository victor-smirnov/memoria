
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
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry)
        {
            if (isOk(status_)) {
                status_ <<= obj->template _insert_b<Offset>(idx, accum, [&](int32_t block) -> const auto& {
                        return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
                });
            }
        }


        template <typename NTypes, typename... Args>
        void treeNode(bt::LeafNode<NTypes>* node, int32_t idx, BranchNodeEntry& accum, Args&&... args)
        {
            node->layout(255);
            node->template processStreamAcc<Stream>(*this, accum, idx, std::forward<Args>(args)...);
        }
    };



    template <int32_t Stream, typename Entry>
    OpStatus try_insert_stream_entry_no_mgr(NodeBaseG& leaf, int32_t idx, const Entry& entry)
    {
        BranchNodeEntry accum;
        InsertStreamEntryFn<Stream> fn;
        self().leaf_dispatcher().dispatch(leaf, fn, idx, accum, entry);

        return fn.status_;
    }


    template <int32_t Stream, typename Entry>
    MMA1_NODISCARD std::tuple<bool> try_insert_stream_entry(Iterator& iter, int32_t idx, const Entry& entry)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.updateBlockG(iter.leaf());

        mgr.add(iter.leaf());

        auto status = self.template try_insert_stream_entry_no_mgr<Stream>(iter.leaf(), idx, entry);

        if (isFail(status))
        {
            mgr.rollback();
            return std::make_tuple(false);
        }

        return std::make_tuple(true);
    }

    MMA1_NODISCARD bool with_block_manager(NodeBaseG& leaf, int structure_idx, int stream_idx, std::function<OpStatus(int, int)> insert_fn)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.updateBlockG(leaf);

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
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, int32_t idx)
        {
            if (isOk(status_)) {
                status_ <<= obj->template _remove<Offset>(idx, accum);
            }
        }

        template <typename NTypes>
        void treeNode(bt::LeafNode<NTypes>* node, int32_t idx, BranchNodeEntry& accum)
        {
            node->layout(255);
            node->template processStreamAcc<Stream>(*this, accum, idx);
        }
    };


    template <int32_t Stream>
    MMA1_NODISCARD std::tuple<bool, BranchNodeEntry> try_remove_stream_entry(Iterator& iter, int32_t idx)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.updateBlockG(iter.leaf());

        mgr.add(iter.leaf());

        BranchNodeEntry accum;
        RemoveFromLeafFn<Stream> fn;
        self.leaf_dispatcher().dispatch(iter.leaf(), fn, idx, accum);

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
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry)
        {
            if (isOk(status_)) {
                status_ <<= obj->template _update_b<Offset>(idx, accum, [&](int32_t block){
                    return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
                });
            }
        }

        template <typename NTypes, typename... Args>
        void treeNode(bt::LeafNode<NTypes>* node, int32_t idx, BranchNodeEntry& accum, Args&&... args)
        {
            node->template processSubstreamsByIdxAcc<
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
    MMA1_NODISCARD std::tuple<bool, BranchNodeEntry> try_update_stream_entry(Iterator& iter, int32_t idx, const Entry& entry)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.updateBlockG(iter.leaf());

        mgr.add(iter.leaf());

        BranchNodeEntry accum;
        UpdateStreamEntryBufferFn<Stream, SubstreamsList> fn;
        self.leaf_dispatcher().dispatch(
                    iter.leaf(),
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
        return self.updateAtomic(iter, std::forward<Fn>(fn), VLSelector(), std::forward<Fn>(args)...);
    }

    // FIXME: not used
    // NodeBaseG createNextLeaf(NodeBaseG& leaf);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(TryMergeNodesFn, mergeWith, OpStatus);
    MMA1_NODISCARD bool tryMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn = [](const Position&){});
    bool mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});
    bool mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
bool M_TYPE::tryMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    BlockUpdateMgr mgr(self);

    self.updateBlockG(src);
    self.updateBlockG(tgt);

    mgr.add(src);
    mgr.add(tgt);

    Position tgt_sizes  = self.getNodeSizes(tgt);

    int32_t tgt_size            = self.getNodeSize(tgt, 0);
    NodeBaseG src_parent        = self.getNodeParent(src);
    int32_t parent_idx          = src->parent_idx();

    MEMORIA_V1_ASSERT(parent_idx, >, 0);

    if (isFail(self.leaf_dispatcher().dispatch(src, tgt, TryMergeNodesFn()))) {
        mgr.rollback();
        return false;
    }

    self.updateChildren(tgt, tgt_size);

    BranchNodeEntry max = self.max(tgt);

    // FIXME. This is apecial OOM condition that, if occurs, must be handled separately.
    OOM_THROW_IF_FAILED(self.removeNonLeafNodeEntry(src_parent, parent_idx), MMA1_SRC);

    int32_t idx = parent_idx - 1;

    self.updateBranchNodes(src_parent, idx, max);

    self.allocator().removeBlock(src->id());

    fn(tgt_sizes);

    return true;
}


M_PARAMS
bool M_TYPE::mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    if (self.isTheSameParent(tgt, src))
    {
        return self.mergeCurrentLeafNodes(tgt, src, fn);
    }
    else
    {
        NodeBaseG tgt_parent = self.getNodeParent(tgt);
        NodeBaseG src_parent = self.getNodeParent(src);

        if (self.mergeBranchNodes(tgt_parent, src_parent))
        {
            return self.mergeCurrentLeafNodes(tgt, src, fn);
        }
        else
        {
            return false;
        }
    }
}




M_PARAMS
bool M_TYPE::mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    if (self.tryMergeLeafNodes(tgt, src, fn))
    {
        self.removeRedundantRootP(tgt);
        return true;
    }
    else {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS

}}
