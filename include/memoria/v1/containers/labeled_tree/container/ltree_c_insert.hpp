
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
#include <memoria/v1/core/types.hpp>
#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_misc_walkers.hpp>

namespace memoria {
namespace v1 {




MEMORIA_V1_CONTAINER_PART_BEGIN(louds::CtrInsertName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;


    using LabelsTuple = typename Types::LabelsTuple;

    typedef typename Base::Types::CtrSizeT                                      CtrSizeT;

    static const int32_t Streams = Types::Streams;


    template <typename SubstreamsIdxList, typename... Args>
    auto iter_read_leaf_entry(const NodeBaseG& leaf, Args&&... args) const
    {
        return self().template ctr_apply_substreams_fn<0, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }

    struct InsertLabelsFn
    {
        template <
            int32_t Offset,
            bool StreamStart,
            int32_t Idx,
            typename BranchNodeEntryItem
        >
        void stream(StreamSize& obj, BranchNodeEntryItem& accum, const LabelsTuple& labels, int32_t idx)
        {
            obj.insert(idx, 1);
            accum[Offset]++;
        }


        template <int32_t Offset, bool StreamStart, int32_t Idx, typename ExtData, typename PkdArray, typename BranchNodeEntryItem>
        void stream(PackedFSEArraySO<ExtData, PkdArray>& array, BranchNodeEntryItem& accum, const LabelsTuple& labels, int32_t idx)
        {
            array.insert(idx, std::get<Idx - 1>(labels));
        }

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamTypes, typename BranchNodeEntryItem>
        void stream(PkdVQTree<StreamTypes>* sizes, BranchNodeEntryItem& accum, const LabelsTuple& labels, int32_t idx)
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
            int32_t Offset,
            bool StreamStart,
            int32_t Idx,
            typename SeqTypes,
            typename BranchNodeEntryItem
        >
        void stream(PkdFSSeq<SeqTypes>* obj, BranchNodeEntryItem& accum, int32_t idx, int32_t symbol)
        {
            obj->insert(idx, symbol);
            accum[Offset + symbol]++;
        }

        template <
            int32_t Offset,
            bool StreamStart,
            int32_t Idx,
            typename BranchNodeEntryItem
        >
        void stream(StreamSize& obj, BranchNodeEntryItem& accum, int32_t idx, int32_t symbol)
        {
            obj.insert(idx, symbol);
            accum[Offset]++;
        }

        template <typename CtrT, typename NTypes>
        void treeNode(
                LeafNodeSO<CtrT, NTypes>& node,
                BranchNodeEntry& delta,
                const LabelsTuple& labels,
                int32_t node_idx,
                int32_t label_idx,
                int32_t symbol
        )
        {
            node->layout(-1ull);
            node->template processStreamAcc<0>(*this, delta, node_idx, symbol);

            InsertLabelsFn fn;
            node->template processStreamAcc<1>(fn, delta, labels, label_idx);
        }

        template <typename CtrT, typename NTypes>
        void treeNode(LeafNodeSO<CtrT, NTypes>* node, BranchNodeEntry& delta, int32_t node_idx)
        {
            node->layout(-1ull);
            node->template processStreamAcc<0>(*this, delta, node_idx, 0);
        }
    };






    bool insertLoudsNode(NodeBaseG& leaf, int32_t node_idx, int32_t label_idx, BranchNodeEntry& sums, const LabelsTuple& labels)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        mgr.add(leaf);

        try {
            self().leaf_dispatcher().dispatch(
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






    bool insertLoudsZero(NodeBaseG& leaf, int32_t node_idx, BranchNodeEntry& sums)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        mgr.add(leaf);

        try {
            self().leaf_dispatcher().dispatch(leaf, InsertNodeFn(), sums, node_idx);
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
        auto& leaf  = iter.iter_leaf();
        int32_t& idx    = iter.iter_local_pos();
        int32_t stream  = iter.iter_stream();
        int32_t size    = iter.iter_leaf_size(stream);

        int32_t split_idx = size / 2;
        int32_t label_idx = iter.label_idx(split_idx);

        auto right = self.ctr_split_leaf(leaf, {split_idx, label_idx});

        if (idx > split_idx)
        {
            leaf = right;
            idx -= split_idx;
        }
    }


    void insertNode(Iterator& iter, const LabelsTuple& labels)
    {
        auto& self  = this->self();
        auto& leaf  = iter.iter_leaf();
        int32_t& idx    = iter.iter_local_pos();

        int32_t label_idx = iter.label_idx();

        BranchNodeEntry sums;

        if (self.insertLoudsNode(leaf, idx, label_idx, sums, labels))
        {
            self.ctr_update_path(leaf);
        }
        else {
            self.split(iter);

            label_idx = iter.label_idx();

            bool result = self.insertLoudsNode(leaf, idx, label_idx, sums, labels);
            MEMORIA_V1_ASSERT_TRUE(result);

            self.ctr_update_path(leaf);
        }
    }

    void insertZero(Iterator& iter)
    {
        auto& self  = this->self();
        auto& leaf  = iter.iter_leaf();
        int32_t& idx    = iter.iter_local_pos();

        BranchNodeEntry sums;

        if (self.insertLoudsZero(leaf, idx, sums))
        {
            self.ctr_update_path(leaf);
        }
        else
        {
            self.split(iter);

            bool result = self.insertLoudsZero(leaf, idx, sums);

            MEMORIA_V1_ASSERT_TRUE(result);

            self.ctr_update_path(leaf);
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

        iter.iter_skip_bw(iter.pos() - pos);
    }




MEMORIA_V1_CONTAINER_PART_END

}}
