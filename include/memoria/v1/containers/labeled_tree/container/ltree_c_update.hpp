
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


namespace memoria {
namespace v1 {




MEMORIA_V1_CONTAINER_PART_BEGIN(v1::louds::CtrUpdateName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Base::Types::LabelsTuple                                   LabelsTuple;

    static const int32_t Streams                                                    = Types::Streams;

    template <int32_t LabelIdx>
    struct SetLabelValueFn {

        BranchNodeEntry& delta_;


        SetLabelValueFn(BranchNodeEntry& delta):
            delta_(delta)
        {}


        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PackedFSEArray<StreamTypes>* labels, BranchNodeEntryItem& , int32_t idx, T&& value)
        {
            labels->value(0, idx) = value;
        }

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PkdVQTree<StreamTypes>* obj, BranchNodeEntryItem& accum, int32_t idx, T&& value)
        {
            auto delta = obj->setValue1(0, idx, value);
            accum[Offset] += delta;
        }

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PkdFQTree<StreamTypes>* obj, BranchNodeEntryItem& accum, int32_t idx, T&& value)
        {
            auto delta = obj->setValue(0, idx, value);

            accum[Offset] += delta;
        }


        template <typename Node, typename T>
        void treeNode(Node* node, int32_t label_idx, T&& value)
        {
            node->layout(-1);
            node->template processStreamAccP<IntList<1, 1, LabelIdx>>(*this, delta_, label_idx, std::forward<T>(value));
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

    template <int32_t LabelIdx, typename T>
    void setLabel(Iterator& iter, T&& value)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();

        int32_t label_idx = iter.label_idx();

        BranchNodeEntry sums;

        if (self.updateNodeLabel(leaf, SetLabelValueFn<LabelIdx>(sums), label_idx, value))
        {
            self.update_path(leaf);
        }
        else
        {
            self.split(iter);

            label_idx = iter.label_idx();

            auto result = self.updateNodeLabel(leaf, SetLabelValueFn<LabelIdx>(sums), label_idx, value);

            MEMORIA_V1_ASSERT_TRUE(result);
            self.update_path(leaf);
        }
    }



    template <int32_t LabelIdx>
    struct AddLabelValueFn {

        BranchNodeEntry& delta_;

        AddLabelValueFn(BranchNodeEntry& delta):
            delta_(delta)
        {}


        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PackedFSEArray<StreamTypes>* labels, BranchNodeEntryItem& accum, int32_t idx, T&& value)
        {
            labels->value(0, idx) += value;
        }

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PkdVQTree<StreamTypes>* obj, BranchNodeEntryItem& accum, int32_t idx, T&& value)
        {
            obj->addValue(0, idx, value);
            accum[Offset] += value;
        }

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamTypes, typename BranchNodeEntryItem, typename T>
        void stream(PkdFQTree<StreamTypes>* obj, BranchNodeEntryItem& accum, int32_t idx, T&& value)
        {
            obj->addValue(0, idx, value);
            accum[Offset] += value;
        }

        template <typename Node, typename T>
        void treeNode(Node* node, int32_t label_idx, T&& value)
        {
            node->layout(-1);
            node->template processStreamAccP<IntList<1, 1, LabelIdx>>(*this, delta_, label_idx, std::forward<T>(value));
        }
    };


    template <int32_t LabelIdx, typename T>
    void addLabel(Iterator& iter, T&& value)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();

        int32_t label_idx = iter.label_idx();

        BranchNodeEntry sums;

        if (self.updateNodeLabel(leaf, AddLabelValueFn<LabelIdx>(sums), label_idx, value))
        {
            self.update_path(leaf);
        }
        else
        {
            self.split(iter);

            label_idx = iter.label_idx();

            auto result = self.updateNodeLabel(leaf, AddLabelValueFn<LabelIdx>(sums), label_idx, value);

            MEMORIA_V1_ASSERT_TRUE(result);

            self.update_path(leaf);
        }
    }

MEMORIA_V1_CONTAINER_PART_END

}}
