
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_names.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_tools.hpp>
#include <memoria/v1/containers/seq_dense/seqd_walkers.hpp>


namespace memoria {




MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrUpdateName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Base::Types::LabelsTuple                                   LabelsTuple;

    static const Int Streams                                                    = Types::Streams;

    template <Int LabelIdx>
    struct SetLabelValueFn {

        BranchNodeEntry& delta_;


        SetLabelValueFn(BranchNodeEntry& delta):
            delta_(delta)
        {}


        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PackedFSEArray<StreamTypes>* labels, BranchNodeEntryItem& , Int idx, T&& value)
        {
            labels->value(0, idx) = value;
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PkdVQTree<StreamTypes>* obj, BranchNodeEntryItem& accum, Int idx, T&& value)
        {
            auto delta = obj->setValue1(0, idx, value);
            accum[Offset] += delta;
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PkdFQTree<StreamTypes>* obj, BranchNodeEntryItem& accum, Int idx, T&& value)
        {
            auto delta = obj->setValue(0, idx, value);

            accum[Offset] += delta;
        }


        template <typename Node, typename T>
        void treeNode(Node* node, Int label_idx, T&& value)
        {
            node->layout(-1);
            node->template processStreamAccP<IntList<1, 0, LabelIdx>>(*this, delta_, label_idx, std::forward<T>(value));
        }
    };


    template <typename Fn, typename... Args>
    bool updateNodeLabel(
            NodeBaseG& leaf,
            Fn&& fn,
            Args&&... args
    )
    {
        auto& self = this->self();

        PageUpdateMgr mgr(self);

        mgr.add(leaf);

        try {
            LeafDispatcher::dispatch(leaf, std::forward<Fn>(fn), std::forward<Args>(args)...);
            return true;
        }
        catch (PackedOOMException& e)
        {
            mgr.rollback();
            return false;
        }
    }

    template <Int LabelIdx, typename T>
    void setLabel(Iterator& iter, T&& value)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();

        Int label_idx = iter.label_idx();

        BranchNodeEntry sums;

        if (self.updateNodeLabel(leaf, SetLabelValueFn<LabelIdx>(sums), label_idx, value))
        {
            self.update_parent(leaf, sums);
        }
        else
        {
            self.split(iter);

            label_idx = iter.label_idx();

            auto result = self.updateNodeLabel(leaf, SetLabelValueFn<LabelIdx>(sums), label_idx, value);

            MEMORIA_ASSERT_TRUE(result);
            self.update_parent(leaf, sums);
        }
    }



    template <Int LabelIdx>
    struct AddLabelValueFn {

        BranchNodeEntry& delta_;

        AddLabelValueFn(BranchNodeEntry& delta):
            delta_(delta)
        {}


        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PackedFSEArray<StreamTypes>* labels, BranchNodeEntryItem& accum, Int idx, T&& value)
        {
            labels->value(0, idx) += value;
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PkdVQTree<StreamTypes>* obj, BranchNodeEntryItem& accum, Int idx, T&& value)
        {
            obj->addValue(0, idx, value);
            accum[Offset] += value;
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PkdFQTree<StreamTypes>* obj, BranchNodeEntryItem& accum, Int idx, T&& value)
        {
            obj->addValue(0, idx, value);
            accum[Offset] += value;
        }

        template <typename Node, typename T>
        void treeNode(Node* node, Int label_idx, T&& value)
        {
            node->layout(-1);
            node->template processStreamAccP<IntList<1, 0, LabelIdx>>(*this, delta_, label_idx, std::forward<T>(value));
        }
    };


    template <Int LabelIdx, typename T>
    void addLabel(Iterator& iter, T&& value)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();

        Int label_idx = iter.label_idx();

        BranchNodeEntry sums;

        if (self.updateNodeLabel(leaf, AddLabelValueFn<LabelIdx>(sums), label_idx, value))
        {
            self.update_parent(leaf, sums);
        }
        else
        {
            self.split(iter);

            label_idx = iter.label_idx();

            auto result = self.updateNodeLabel(leaf, AddLabelValueFn<LabelIdx>(sums), label_idx, value);

            MEMORIA_ASSERT_TRUE(result);
            self.update_parent(leaf, sums);
        }
    }

MEMORIA_CONTAINER_PART_END

}
