
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/prototypes/bt_cow/btcow_names.hpp>

#include <vector>

namespace memoria {
namespace v1 {


MEMORIA_V1_CONTAINER_PART_BEGIN(btcow::LeafVariableName)
public:
    using typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef std::function<void (const Position&)>                               MergeFn;

    static const int32_t Streams                                                    = Types::Streams;

public:
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
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry)
        {
            obj->template _insert_b<Offset>(idx, accum, [&](int32_t block) -> const auto& {
                return entry.get(StreamTag<Stream>(), StreamTag<Idx>(), block);
            });
        }


        template <typename NTypes, typename... Args>
        void treeNode(LeafNode<NTypes>* node, int32_t idx, BranchNodeEntry& accum, Args&&... args)
        {
            node->layout(255);
            node->template processStreamAcc<Stream>(*this, accum, idx, std::forward<Args>(args)...);
        }
    };




    template <int32_t Stream, typename Entry>
    std::tuple<bool> try_insert_stream_entry(Iterator& iter, int32_t idx, const Entry& entry)
    {
        auto& self = this->self();

        PageUpdateMgr mgr(self);

        self.updateBlockG(iter.leaf());

        mgr.add(iter.leaf());

        try {
            BranchNodeEntry accum;
            LeafDispatcher::dispatch(iter.leaf(), InsertStreamEntryFn<Stream>(), idx, accum, entry);
            return std::make_tuple(true);
        }
        catch (PackedOOMException& e)
        {
            mgr.rollback();
            return std::make_tuple(false);
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
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, int32_t idx)
        {
            obj->template _remove<Offset>(idx, accum);
        }

        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, int32_t idx, BranchNodeEntry& accum)
        {
            node->layout(255);
            node->template processStreamAcc<Stream>(*this, accum, idx);
        }
    };


    template <int32_t Stream>
    std::tuple<bool, BranchNodeEntry> try_remove_stream_entry(Iterator& iter, int32_t idx)
    {
        auto& self = this->self();

        PageUpdateMgr mgr(self);

        self.updateBlockG(iter.leaf());

        mgr.add(iter.leaf());

        try {
            BranchNodeEntry accum;
            LeafDispatcher::dispatch(iter.leaf(), RemoveFromLeafFn<Stream>(), idx, accum);
            return std::make_tuple(true, accum);
        }
        catch (PackedOOMException& e)
        {
            mgr.rollback();
            return std::make_tuple(false, BranchNodeEntry());
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
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, int32_t idx, const Entry& entry)
        {
            obj->template _update_b<Offset>(idx, accum, [&](int32_t block){
                return entry.get(StreamTag<Stream>(), StreamTag<Idx>(), block);
            });
        }

        template <typename NTypes, typename... Args>
        void treeNode(LeafNode<NTypes>* node, int32_t idx, BranchNodeEntry& accum, Args&&... args)
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
    std::tuple<bool, BranchNodeEntry> try_update_stream_entry(Iterator& iter, int32_t idx, const Entry& entry)
    {
        auto& self = this->self();

        PageUpdateMgr mgr(self);

        self.updateBlockG(iter.leaf());

        mgr.add(iter.leaf());

        try {
            BranchNodeEntry accum;
            LeafDispatcher::dispatch(
                    iter.leaf(),
                    UpdateStreamEntryBufferFn<Stream, SubstreamsList>(),
                    idx,
                    accum,
                    entry
            );
            return std::make_tuple(true, accum);
        }
        catch (PackedOOMException& e)
        {
            mgr.rollback();
            return std::make_tuple(false, BranchNodeEntry());
        }
    }

    template <typename Fn, typename... Args>
    bool update(Iterator& iter, Fn&& fn, Args&&... args)
    {
        auto& self = this->self();
        return self.updateAtomic(iter, std::forward<Fn>(fn), VLSelector(), std::forward<Fn>(args)...);
    }

    // FIXME: not used
    NodeBaseG createNextLeaf(NodeBaseG& leaf);

    MEMORIA_V1_DECLARE_NODE_FN(TryMergeNodesFn, mergeWith);
    bool tryMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn = [](const Position&){});
    bool mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});
    bool mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btcow::LeafVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::createNextLeaf(NodeBaseG& left_node)
{
    auto& self = this->self();

    if (left_node->is_root())
    {
        self.newRootP(left_node);
    }
    else {
        self.updateBlockG(left_node);
    }

    NodeBaseG left_parent  = self.getNodeParentForUpdate(left_node);

    NodeBaseG other  = self.createNode1(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

    BranchNodeEntry sums;

    int32_t parent_idx = left_node->parent_idx();

    PageUpdateMgr mgr(self);
    mgr.add(left_parent);

    try {
        self.insertNonLeafP(left_parent, parent_idx + 1, sums, other->id());
    }
    catch (PackedOOMException ex)
    {
        mgr.rollback();

        NodeBaseG right_parent = splitPathP(left_parent, parent_idx + 1);

        mgr.add(right_parent);

        try {
            self.insertNonLeafP(right_parent, 0, sums, other->id());
        }
        catch (PackedOOMException ex2)
        {
            mgr.rollback();

            int32_t right_parent_size = self.getNodeSize(right_parent, 0);

            splitPathP(right_parent, right_parent_size / 2);

            self.insertNonLeafP(right_parent, 0, sums, other->id());
        }
    }

    return other;
}


M_PARAMS
bool M_TYPE::tryMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);

    self.updateBlockG(src);
    self.updateBlockG(tgt);

    mgr.add(src);
    mgr.add(tgt);

    Position tgt_sizes  = self.getNodeSizes(tgt);

    try {
        int32_t tgt_size            = self.getNodeSize(tgt, 0);
        NodeBaseG src_parent    = self.getNodeParent(src);
        int32_t parent_idx          = src->parent_idx();

        MEMORIA_V1_ASSERT(parent_idx, >, 0);

        LeafDispatcher::dispatch(src, tgt, TryMergeNodesFn());

        self.updateChildren(tgt, tgt_size);

        BranchNodeEntry max = self.max(tgt);

        self.removeNonLeafNodeEntry(src_parent, parent_idx);

        int32_t idx = parent_idx - 1;

        self.updateBranchNodes(src_parent, idx, max);

        self.allocator().removeBlock(src->id(), self.master_name());

        fn(tgt_sizes);

        return true;
    }
    catch (PackedOOMException ex)
    {
        mgr.rollback();
    }

    return false;
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
