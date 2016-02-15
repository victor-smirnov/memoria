
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LBLTREE_C_INSERT_HPP
#define MEMORIA_CONTAINERS_LBLTREE_C_INSERT_HPP

#include <memoria/containers/labeled_tree/ltree_names.hpp>
#include <memoria/containers/labeled_tree/tools/ltree_tree_tools.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/packed/array/packed_fse_array.hpp>
#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/prototypes/bt/walkers/bt_misc_walkers.hpp>

namespace memoria    {




MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrInsertName)

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


    template <typename SubstreamsIdxList, typename... Args>
    auto read_leaf_entry(const NodeBaseG& leaf, Args&&... args) const
    {
    	return self().template apply_substreams_fn<0, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }

    struct InsertLabelsFn
    {
        const LabelsTuple& labels_;

        InsertLabelsFn(const LabelsTuple& labels):
        	labels_(labels)
        {}

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem>
        void stream(PackedFSEArray<StreamTypes>* labels, BranchNodeEntryItem& accum, Int idx)
        {
            labels->insert(idx, std::get<Idx>(labels_));

            if (StreamStart)
            {
            	accum[0] += 1;
            }
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem>
        void stream(PkdVQTree<StreamTypes>* sizes, BranchNodeEntryItem& accum, Int idx)
        {
            typedef typename PkdVQTree<StreamTypes>::Values Values;

            auto size = std::get<Idx>(labels_);

            Values values;
            values[0] = size;

            sizes->insert(idx, values);

            if (StreamStart)
            {
            	accum[0] += 1;
            }

            accum[Offset] += size;
        }
    };




    struct InsertNodeFn {
        BranchNodeEntry& delta_;
        const LabelsTuple& labels_;

        InsertNodeFn(BranchNodeEntry& delta, const LabelsTuple& labels):
            delta_(delta),
            labels_(labels)
        {}

        template <Int Idx, typename SeqTypes>
        void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Int symbol)
        {
            MEMORIA_ASSERT_TRUE(seq != nullptr);

            seq->insert(idx, symbol);

            std::get<Idx>(delta_)[0]++;
            std::get<Idx>(delta_)[symbol + 1]++;
        }

        template <typename NTypes, typename... Labels>
        void treeNode(LeafNode<NTypes>* node, Int node_idx, Int label_idx, Int symbol)
        {
            node->layout(-1);
            node->template processStream<IntList<0>>(*this, node_idx, symbol);

            InsertLabelsFn fn(labels_);
            node->template processStreamAcc<1>(fn, delta_, label_idx);
        }

        template <typename NTypes, typename... Labels>
        void treeNode(LeafNode<NTypes>* node, Int node_idx)
        {
            node->layout(1);
            node->template processStream<IntList<0>>(*this, node_idx, 0);
        }
    };






    bool insertLoudsNode(NodeBaseG& leaf, Int node_idx, Int label_idx, BranchNodeEntry& sums, const LabelsTuple& labels)
    {
    	auto& self = this->self();

        PageUpdateMgr mgr(self);

        mgr.add(leaf);

        try {
            LeafDispatcher::dispatch(
                    leaf,
                    InsertNodeFn(sums, labels),
                    node_idx,
                    label_idx,
                    1
            );

            return true;
        }
        catch (PackedOOMException& e)
        {
            Clear(sums);
            mgr.rollback();
            return false;
        }
    }






    bool insertLoudsZero(NodeBaseG& leaf, Int node_idx, BranchNodeEntry& sums)
    {
        auto& self = this->self();

        PageUpdateMgr mgr(self);

        mgr.add(leaf);

        try {
            LeafDispatcher::dispatch(leaf, InsertNodeFn(sums, LabelsTuple()), node_idx);
            return true;
        }
        catch (PackedOOMException& e)
        {
            Clear(sums);
            mgr.rollback();
            return false;
        }
    }


    void split(Iterator& iter)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();
        Int& idx    = iter.idx();
        Int stream  = iter.stream();
        Int size    = iter.leaf_size(stream);

        Int split_idx = size / 2;
        Int label_idx = iter.label_idx(split_idx);

        auto right = self.split_leaf_p(leaf, {split_idx, label_idx});

        if (idx > split_idx)
        {
            leaf = right;
            idx -= split_idx;
        }
    }


    void insertNode(Iterator& iter, const LabelsTuple& labels)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();
        Int& idx    = iter.idx();

        Int label_idx = iter.label_idx();

        BranchNodeEntry sums;

        if (self.insertLoudsNode(leaf, idx, label_idx, sums, labels))
        {
            self.update_parent(leaf, sums);
        }
        else {
        	self.split(iter);

            label_idx = iter.label_idx();

            bool result = self.insertLoudsNode(leaf, idx, label_idx, sums, labels);
            MEMORIA_ASSERT_TRUE(result);
            self.update_parent(leaf, sums);
        }
    }

    void insertZero(Iterator& iter)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();
        Int& idx    = iter.idx();


        BranchNodeEntry sums;

        if (self.insertLoudsZero(leaf, idx, sums))
        {
            self.update_parent(leaf, sums);
        }
        else
        {
            self.split(iter);

            bool result = self.insertLoudsZero(leaf, idx, sums);

            MEMORIA_ASSERT_TRUE(result);
            self.update_parent(leaf, sums);
        }
    }

    LoudsNode newNodeAt(const LoudsNode& node, const LabelsTuple& labels)
    {
        auto& self = this->self();

        auto iter = self.findNode(node);

        self.insertNode(*iter.get(), labels);

        iter = self.firstChild(iter->node());

        self.insertZero(*iter.get());

        return iter->node();
    }

    void newNodeAt(Iterator& iter, const LabelsTuple& labels)
    {
        auto& self = this->self();

        CtrSizeT pos = iter.pos();

        self.insertNode(iter, labels);

        iter.firstChild();

        self.insertZero(iter);

        iter.skipBw(iter.pos() - pos);
    }




MEMORIA_CONTAINER_PART_END

}


#endif
