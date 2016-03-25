
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_names.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_tools.hpp>
#include <memoria/v1/containers/seq_dense/seqd_walkers.hpp>


#include <memoria/v1/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::louds::CtrRemoveName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Base::Types::LabelsTuple                                   LabelsTuple;

    typedef typename Base::Types::CtrSizeT                                      CtrSizeT;

    static const Int Streams                                                    = Types::Streams;


    struct RemoveFromLeafFn {
        BranchNodeEntry& delta_;
        Position sizes_;

        Int label_idx_;

        RemoveFromLeafFn(BranchNodeEntry& delta): delta_(delta) {}

        template <Int Offset, bool StreamStart, Int Idx, typename SeqTypes, typename BranchNodeEntryItem>
        void stream(PkdFSSeq<SeqTypes>* seq, BranchNodeEntryItem& accum, Int idx)
        {
            MEMORIA_V1_ASSERT_TRUE(seq != nullptr);

            Int sym = seq->symbol(idx);

            if (sym) {
                label_idx_ = seq->rank(idx, 1);
            }
            else {
                label_idx_ = -1;
            }

            seq->remove(idx, idx + 1);

            accum[0] -= 1;
            accum[Offset + sym] -= 1;

            sizes_[Idx] = -1;
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem>
        void stream(PackedFSEArray<StreamTypes>* labels, BranchNodeEntryItem& accum, Int idx)
        {
            if (label_idx_ >= 0)
            {
                labels->remove(label_idx_, label_idx_ + 1);

                sizes_[Idx] = -1;

                if (StreamStart)
                {
                    accum[0] -= 1;
                }
            }
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem>
        void stream(PkdVQTree<StreamTypes>* sizes, BranchNodeEntryItem& accum, Int idx)
        {
            if (label_idx_ >= 0)
            {
                auto size = sizes->value(0, label_idx_);

                sizes->remove(label_idx_, label_idx_ + 1);

                if (StreamStart)
                {
                    accum[0] -= 1;
                }

                accum[Offset] -= size;

                sizes_[Idx] = -1;
            }
        }



        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx)
        {
            node->template processStreamAcc<0>(*this, delta_, idx);
            node->template processStreamAcc<1>(*this, delta_, idx);
        }
    };

    Position removeFromLeaf(NodeBaseG& leaf, Int idx, BranchNodeEntry& indexes)
    {
        RemoveFromLeafFn fn(indexes);
        LeafDispatcher::dispatch(leaf, fn, idx);

        return fn.sizes_;
    }

    void remove(CtrSizeT idx)
    {
        auto& self  = this->self();
        auto iter   = self.seek(idx);

        self.remove(iter);
    }

    void remove(Iterator& iter)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();
        Int& idx    = iter.idx();

        BranchNodeEntry sums;

        removeFromLeaf(leaf, idx, sums);

        self.update_parent(leaf, sums);

        self.mergeLeafWithRightSibling(leaf);
    }




    void removeLeaf(const LoudsNode& node)
    {
        auto& self = this->self();

        auto iter = self.findNode(node);

        MEMORIA_V1_ASSERT_TRUE(iter->symbol() == 1);

        iter->firstChild();

        MEMORIA_V1_ASSERT_TRUE(iter->symbol() == 0);

        iter->remove();

        iter = self.findNode(node);
        iter->remove();
    }

    void removeLeaf(Iterator& iter)
    {
        auto& self = this->self();

        CtrSizeT idx = iter.pos();

        MEMORIA_V1_ASSERT_TRUE(iter.symbol() == 1);

        iter.firstChild();

        if (iter.symbol() != 0)
        {
            iter.dumpPath();
        }

        MEMORIA_V1_ASSERT_TRUE(iter.symbol() == 0);

        iter.remove();

        iter = *self.seek(idx).get();
        iter.remove();
    }

MEMORIA_V1_CONTAINER_PART_END

}}