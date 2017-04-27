
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

using namespace v1::bt;
using namespace v1::core;

using namespace std;

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::btcow::LeafFixedName)
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
            obj->template _insert_b<Offset>(idx, accum, [&](Int block) -> const auto& {
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
    std::tuple<bool> try_insert_stream_entry(Iterator& iter, Int idx, const Entry& entry)
    {
        auto& self = this->self();

        self.updatePageG(iter.leaf());

        if (self.checkCapacities(iter.leaf(), Position::create(Stream, 1)))
        {
            BranchNodeEntry accum;
            LeafDispatcher::dispatch(iter.leaf(), InsertStreamEntryFn<Stream>(), idx, accum, entry);
            return std::make_tuple(true);
        }
        else {
            return std::tuple<bool>(false);
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
    std::tuple<bool, BranchNodeEntry> try_remove_stream_entry(Iterator& iter, Int idx)
    {
        BranchNodeEntry accum;
        LeafDispatcher::dispatch(iter.leaf(), RemoveFromLeafFn<Stream>(), idx, accum);
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
            obj->template _update_b<Offset>(idx, accum, [&](Int block) -> const auto& {
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
    std::tuple<bool, BranchNodeEntry> try_update_stream_entry(Iterator& iter, Int idx, const Entry& entry)
    {
        auto& self = this->self();

        self.updatePageG(iter.leaf());

        BranchNodeEntry accum;
        LeafDispatcher::dispatch(
                iter.leaf(),
                UpdateStreamEntryFn<Stream, SubstreamsList>(),
                idx,
                accum,
                entry
        );

        return std::make_tuple(true, accum);
    }





    //==========================================================================================

    MEMORIA_V1_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);
    bool canMerge(const NodeBaseG& tgt, const NodeBaseG& src)
    {
        return NodeDispatcher::dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(MergeNodesFn, mergeWith);
    void doMergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});
    bool mergeCurrentLeafNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&){});

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::btcow::LeafFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


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

    MEMORIA_V1_ASSERT(parent_idx, >, 0);

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

}}