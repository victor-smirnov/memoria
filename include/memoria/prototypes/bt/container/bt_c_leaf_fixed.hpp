
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_LEAF_FIXED_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_LEAF_FIXED_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::LeafFixedName)
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

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef std::function<void (const Position&)>                               MergeFn;

    using CtrSizeT = typename Types::CtrSizeT;

    static const Int Streams                                                    = Types::Streams;

    template <
        Int Stream
    >
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

        self.updatePageG(iter.leaf());

        if (self.checkCapacities(iter.leaf(), Position::create(Stream, 1)))
        {
            BranchNodeEntry accum;
            LeafDispatcher::dispatch(iter.leaf(), InsertStreamEntryFn<Stream>(), iter.idx(), accum, entry);
            return std::make_tuple(true, accum);
        }
        else {
            return std::tuple<bool, BranchNodeEntry>(false, BranchNodeEntry());
        }
    }








    template <Int Stream>
    struct RemoveFromLeafFn
    {
        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx, BranchNodeEntry& accum)
        {
            node->layout(255);
            node->template processStreamAcc<Stream>(*this, accum, idx);
        }

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
    };

    template <Int Stream>
    std::tuple<bool, BranchNodeEntry> try_remove_stream_entry(Iterator& iter)
    {
        BranchNodeEntry accum;
        LeafDispatcher::dispatch(iter.leaf(), RemoveFromLeafFn<Stream>(), iter.idx(), accum);
        return std::make_tuple(true, accum);
    }






    //=========================================================================================


    template <typename Fn, typename... Args>
    bool update(Iterator& iter, Fn&& fn, Args&&... args)
    {
        auto& self = this->self();
        LeafDispatcher::dispatch(
                iter.leaf(),
                fn,
                FLSelector(),
                std::forward<Args>(args)...
        );

        return true;
    }




    template <Int Stream, typename SubstreamsList>
    struct UpdateStreamEntryFn
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

        self.updatePageG(iter.leaf());

        BranchNodeEntry accum;
        LeafDispatcher::dispatch(
                iter.leaf(),
                UpdateStreamEntryFn<Stream, SubstreamsList>(),
                iter.idx(),
                accum,
                entry
        );

        return std::make_tuple(true, accum);
    }





    //==========================================================================================

    MEMORIA_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);
    bool canMerge(const NodeBaseG& tgt, const NodeBaseG& src)
    {
        return NodeDispatcher::dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_DECLARE_NODE_FN(MergeNodesFn, mergeWith);
    void doMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});
    bool mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafFixedName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::doMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    self.updatePageG(tgt);
    self.updatePageG(src);

    Int tgt_size = self.getNodeSize(tgt, 0);

    LeafDispatcher::dispatch(src, tgt, MergeNodesFn());

    self.updateChildren(tgt, tgt_size);

    NodeBaseG src_parent    = self.getNodeParent(src);
    Int parent_idx          = src->parent_idx();

    MEMORIA_ASSERT(parent_idx, >, 0);

    self.removeNonLeafNodeEntry(src_parent, parent_idx);

    Int idx = parent_idx - 1;

    auto max = self.max(tgt);

    self.updateBranchNodes(src_parent, idx, max);

    self.allocator().removePage(src->id(), self.master_name());
}


M_PARAMS
bool M_TYPE::mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    if (self.canMerge(tgt, src))
    {
        if (self.isTheSameParent(tgt, src))
        {
            auto sizes = self.getNodeSizes(tgt);

            self.doMergeLeafNodes(tgt, src);

            self.removeRedundantRootP(tgt);

            fn(sizes);

            return true;
        }
        else
        {
            NodeBaseG tgt_parent = self.getNodeParent(tgt);
            NodeBaseG src_parent = self.getNodeParent(src);

            if (self.mergeBranchNodes(tgt_parent, src_parent))
            {
                auto sizes = self.getNodeSizes(tgt);

                self.doMergeLeafNodes(tgt, src);

                self.removeRedundantRootP(tgt);

                fn(sizes);

                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
}



M_PARAMS
bool M_TYPE::mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
    auto& self = this->self();

    if (self.canMerge(tgt, src))
    {
        fn(self.getNodeSizes(tgt));

        self.doMergeLeafNodes(tgt, src);

        self.removeRedundantRootP(tgt);

        return true;
    }
    else
    {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS

}



#endif
