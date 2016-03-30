
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

#include <memoria/v1/containers/labeled_tree/ltree_names.hpp>
#include <memoria/v1/containers/labeled_tree/tools/ltree_tree_tools.hpp>
#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_misc_walkers.hpp>

namespace memoria {
namespace v1 {




MEMORIA_V1_CONTAINER_PART_BEGIN(v1::louds::CtrInsertName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;


    using LabelsTuple = typename Types::LabelsTuple;

    typedef typename Base::Types::CtrSizeT                                      CtrSizeT;

    static const Int Streams                                                    = Types::Streams;


    template <typename SubstreamsIdxList, typename... Args>
    auto read_leaf_entry(const NodeBaseG& leaf, Args&&... args) const
    {
        return self().template apply_substreams_fn<0, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }

    struct InsertLabelsFn
    {
        template <
            Int Offset,
            bool StreamStart,
            Int Idx,
            typename BranchNodeEntryItem
        >
        void stream(StreamSize* obj, BranchNodeEntryItem& accum, const LabelsTuple& labels, Int idx)
        {
        	obj->insert(idx, 1);
        	accum[Offset]++;
        }


        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem>
        void stream(PackedFSEArray<StreamTypes>* array, BranchNodeEntryItem& accum, const LabelsTuple& labels, Int idx)
        {
            array->insert(idx, std::get<Idx - 1>(labels));
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem>
        void stream(PkdVQTree<StreamTypes>* sizes, BranchNodeEntryItem& accum, const LabelsTuple& labels, Int idx)
        {
            typedef typename PkdVQTree<StreamTypes>::Values Values;

            auto size = std::get<Idx - 1>(labels);

            Values values;
            values[0] = size;

            sizes->insert(idx, values);
            accum[Offset] += size;
        }
    };




    struct InsertNodeFn {
        template <
            Int Offset,
            bool StreamStart,
            Int Idx,
            typename SeqTypes,
            typename BranchNodeEntryItem
        >
        void stream(PkdFSSeq<SeqTypes>* obj, BranchNodeEntryItem& accum, Int idx, Int symbol)
        {
        	obj->insert(idx, symbol);
        	accum[Offset + symbol]++;
        }

        template <
            Int Offset,
            bool StreamStart,
            Int Idx,
            typename BranchNodeEntryItem
        >
        void stream(StreamSize* obj, BranchNodeEntryItem& accum, Int idx, Int symbol)
        {
        	obj->insert(idx, symbol);
        	accum[Offset]++;
        }

        template <typename NTypes>
        void treeNode(
        		LeafNode<NTypes>* node,
				BranchNodeEntry& delta,
				const LabelsTuple& labels,
				Int node_idx,
				Int label_idx,
				Int symbol
		)
        {
            node->layout(-1ull);
            node->template processStreamAcc<0>(*this, delta, node_idx, symbol);

            InsertLabelsFn fn;
            node->template processStreamAcc<1>(fn, delta, labels, label_idx);
        }

        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, BranchNodeEntry& delta, Int node_idx)
        {
            node->layout(-1ull);
            node->template processStreamAcc<0>(*this, delta, node_idx, 0);
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
                    InsertNodeFn(),
					sums,
					labels,
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
            LeafDispatcher::dispatch(leaf, InsertNodeFn(), sums, node_idx);
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
            self.update_path(leaf);
        }
        else {
            self.split(iter);

            label_idx = iter.label_idx();

            bool result = self.insertLoudsNode(leaf, idx, label_idx, sums, labels);
            MEMORIA_V1_ASSERT_TRUE(result);

            self.update_path(leaf);
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
        	self.update_path(leaf);
        }
        else
        {
            self.split(iter);

            bool result = self.insertLoudsZero(leaf, idx, sums);

            MEMORIA_V1_ASSERT_TRUE(result);

            self.update_path(leaf);
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




MEMORIA_V1_CONTAINER_PART_END

}}
