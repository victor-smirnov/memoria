
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LBLTREE_C_REMOVE_HPP
#define MEMORIA_CONTAINERS_LBLTREE_C_REMOVE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/labeled_tree/ltree_names.hpp>
#include <memoria/containers/labeled_tree/ltree_tools.hpp>
#include <memoria/containers/seq_dense/seqd_walkers.hpp>


#include <memoria/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrRemoveName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Base::Types::LabelsTuple                                   LabelsTuple;

    typedef typename Base::Types::CtrSizeT                                      CtrSizeT;

    static const Int Streams                                                    = Types::Streams;


    struct RemoveFromLeafFn {
        Accumulator& delta_;
        Position sizes_;

        Int label_idx_;

        RemoveFromLeafFn(Accumulator& delta): delta_(delta) {}

        template <Int Offset, bool StreamStart, Int Idx, typename SeqTypes, typename AccumulatorItem>
        void stream(PkdFSSeq<SeqTypes>* seq, AccumulatorItem& accum, Int idx)
        {
            MEMORIA_ASSERT_TRUE(seq != nullptr);

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

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename AccumulatorItem>
        void stream(PackedFSEArray<StreamTypes>* labels, AccumulatorItem& accum, Int idx)
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

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename AccumulatorItem>
        void stream(PkdVQTree<StreamTypes>* sizes, AccumulatorItem& accum, Int idx)
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

    Position removeFromLeaf(NodeBaseG& leaf, Int idx, Accumulator& indexes)
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

        Accumulator sums;

        removeFromLeaf(leaf, idx, sums);

        self.update_parent(leaf, sums);

        self.mergeLeafWithRightSibling(leaf);
    }




    void removeLeaf(const LoudsNode& node)
    {
        auto& self = this->self();

        Iterator iter = self.findNode(node);

        MEMORIA_ASSERT_TRUE(iter.symbol() == 1);

        iter.firstChild();

        MEMORIA_ASSERT_TRUE(iter.symbol() == 0);

        iter.remove();

        iter = self.findNode(node);
        iter.remove();
    }

    void removeLeaf(Iterator& iter)
    {
        auto& self = this->self();

        CtrSizeT idx = iter.pos();

        MEMORIA_ASSERT_TRUE(iter.symbol() == 1);

        iter.firstChild();

        if (iter.symbol() != 0)
        {
            iter.dumpPath();
        }

        MEMORIA_ASSERT_TRUE(iter.symbol() == 0);

        iter.remove();

        iter = self.seek(idx);
        iter.remove();
    }

MEMORIA_CONTAINER_PART_END

}


#endif
