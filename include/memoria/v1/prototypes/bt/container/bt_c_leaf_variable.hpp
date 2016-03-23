
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::LeafVariableName)
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

    static const Int Streams                                                    = Types::Streams;

    template <Int Stream>
    struct InsertStreamEntryFn
    {
        template <
            Int Offset,
            bool StreamStart,
            Int Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem,
            typename Entry
        >
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, Int idx, const Entry& entry)
        {
            obj->template _insert_b<Offset>(idx, accum, [&](Int block){
                return entry.get(StreamTag<Stream>(), StreamTag<Idx>(), block);
            });
        }


        template <typename NTypes, typename... Args>
        void treeNode(LeafNode<NTypes>* node, Int idx, BranchNodeEntry& accum, Args&&... args)
        {
            node->layout(255);
            node->template processStreamAcc<Stream>(*this, accum, idx, std::forward<Args>(args)...);
        }
    };




    template <Int Stream, typename Entry>
    std::tuple<bool, BranchNodeEntry> try_insert_stream_entry(Iterator& iter, const Entry& entry)
    {
        auto& self = this->self();

        PageUpdateMgr mgr(self);

        self.updatePageG(iter.leaf());

        mgr.add(iter.leaf());

        try {
            BranchNodeEntry accum;
            LeafDispatcher::dispatch(iter.leaf(), InsertStreamEntryFn<Stream>(), iter.idx(), accum, entry);
            return std::make_tuple(true, accum);
        }
        catch (PackedOOMException& e)
        {
            mgr.rollback();
            return std::make_tuple(false, BranchNodeEntry());
        }
    }






    template <Int Stream>
    struct RemoveFromLeafFn
    {
        template <
            Int Offset,
            bool StreamStart,
            Int Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem
        >
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, Int idx)
        {
            obj->template _remove<Offset>(idx, accum);
        }

        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx, BranchNodeEntry& accum)
        {
            node->layout(255);
            node->template processStreamAcc<Stream>(*this, accum, idx);
        }
    };


    template <Int Stream>
    std::tuple<bool, BranchNodeEntry> try_remove_stream_entry(Iterator& iter)
    {
        auto& self = this->self();

        PageUpdateMgr mgr(self);

        self.updatePageG(iter.leaf());

        mgr.add(iter.leaf());

        try {
            BranchNodeEntry accum;
            LeafDispatcher::dispatch(iter.leaf(), RemoveFromLeafFn<Stream>(), iter.idx(), accum);
            return std::make_tuple(true, accum);
        }
        catch (PackedOOMException& e)
        {
            mgr.rollback();
            return std::make_tuple(false, BranchNodeEntry());
        }
    }




    //=========================================================================================

    template <Int Stream, typename SubstreamsList>
    struct UpdateStreamEntryBufferFn
    {
        template <
            Int Offset,
            bool Start,
            Int Idx,
            typename SubstreamType,
            typename BranchNodeEntryItem,
            typename Entry
        >
        void stream(SubstreamType* obj, BranchNodeEntryItem& accum, Int idx, const Entry& entry)
        {
            obj->template _update_b<Offset>(idx, accum, [&](Int block){
                return entry.get(StreamTag<Stream>(), StreamTag<Idx>(), block);
            });
        }

        template <typename NTypes, typename... Args>
        void treeNode(LeafNode<NTypes>* node, Int idx, BranchNodeEntry& accum, Args&&... args)
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


    template <Int Stream, typename SubstreamsList, typename Entry>
    std::tuple<bool, BranchNodeEntry> try_update_stream_entry(Iterator& iter, const Entry& entry)
    {
        auto& self = this->self();

        PageUpdateMgr mgr(self);

        self.updatePageG(iter.leaf());

        mgr.add(iter.leaf());

        try {
            BranchNodeEntry accum;
            LeafDispatcher::dispatch(
                    iter.leaf(),
                    UpdateStreamEntryBufferFn<Stream, SubstreamsList>(),
                    iter.idx(),
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

    MEMORIA_DECLARE_NODE_FN(TryMergeNodesFn, mergeWith);
    bool tryMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn = [](const Position&){});
    bool mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});
    bool mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafVariableName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::createNextLeaf(NodeBaseG& left_node)
{
    auto& self = this->self();

    if (left_node->is_root())
    {
        self.newRootP(left_node);
    }
    else {
        self.updatePageG(left_node);
    }

    NodeBaseG left_parent  = self.getNodeParentForUpdate(left_node);

    NodeBaseG other  = self.createNode1(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

    BranchNodeEntry sums;

    Int parent_idx = left_node->parent_idx();

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

            Int right_parent_size = self.getNodeSize(right_parent, 0);

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

    self.updatePageG(src);
    self.updatePageG(tgt);

    mgr.add(src);
    mgr.add(tgt);

    Position tgt_sizes  = self.getNodeSizes(tgt);

    try {
        Int tgt_size            = self.getNodeSize(tgt, 0);
        NodeBaseG src_parent    = self.getNodeParent(src);
        Int parent_idx          = src->parent_idx();

        MEMORIA_ASSERT(parent_idx, >, 0);

        LeafDispatcher::dispatch(src, tgt, TryMergeNodesFn());

        self.updateChildren(tgt, tgt_size);

        BranchNodeEntry max = self.max(tgt);

        self.removeNonLeafNodeEntry(src_parent, parent_idx);

        Int idx = parent_idx - 1;

        self.updateBranchNodes(src_parent, idx, max);

        self.allocator().removePage(src->id(), self.master_name());

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

} //memoria
