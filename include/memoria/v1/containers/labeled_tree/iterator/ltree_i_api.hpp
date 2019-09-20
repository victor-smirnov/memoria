
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/containers/labeled_tree/ltree_names.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_tools.hpp>

#include <memoria/v1/core/container/iterator.hpp>

#include <memoria/v1/core/packed/wrappers/louds_tree.hpp>

#include <vector>
#include <iostream>

namespace memoria {
namespace v1 {

MEMORIA_V1_ITERATOR_PART_BEGIN(louds::ItrApiName)
public:
    typedef Ctr<typename Types::CtrTypes>                                       Container;
    typedef Ctr<typename Types::IterTypes>                                      Iterator;

    typedef typename Container::Allocator                                       Allocator;
    typedef typename Container::NodeBaseG                                       NodeBaseG;

    typedef typename Container::BranchNodeEntry                                 BranchNodeEntry;
    typedef typename Container::Position                                        Position;
    typedef typename Container::Types::LabelsTuple                              LabelsTuple;

    typedef typename Container::Types::CtrSizeT                                 CtrSizeT;

    bool operator++() {
        return self().iter_skip_fw(1);
    }

    bool operator--() {
        return self().iter_skip_bw(1);
    }

    bool next() {
        return self().iter_skip_fw(1);
    }

    bool prev() {
        return self().iter_skip_bw(1);
    }


    bool operator++(int) {
        return self().iter_skip_fw(1);
    }

    bool operator--(int) {
        return self().iter_skip_fw(1);
    }

    CtrSizeT operator+=(CtrSizeT size) {
        return self().iter_skip_fw(size);
    }

    CtrSizeT operator-=(CtrSizeT size) {
        return self().iter_skip_bw(size);
    }

    int32_t size() const
    {
        return self().iter_leaf_size(0);
    }

    bool isEof() const {
        return self().iter_local_pos() >= self().size();
    }

    bool isBof() const {
        return self().iter_local_pos() < 0;
    }

    CtrSizeT iter_skip_fw(CtrSizeT amount) {
        return self().template iter_skip_fw<0>(amount);
    }

    CtrSizeT iter_skip_bw(CtrSizeT amount) {
        return self().template iter_skip_bw<0>(amount);
    }

    CtrSizeT skip(CtrSizeT amount) {
        return self().template iter_skip<0>(amount);
    }

    CtrSizeT pos() const
    {
        auto& self = this->self();

        return self.iter_local_pos() + self.iter_cache().size_prefix()[0];
    }

    CtrSizeT noderank_() const
    {
        auto& self = this->self();
//      return self.iter_cache().rank1() + (self.symbol() == 1);

        return self.ranki(1);
    }

    bool isRoot() const
    {
        return self().pos() == 0;
    }

    int32_t value() const
    {
        if (!self().isEof())
        {
            return self().symbol();
        }
        else {
            return 0;
        }
    }

    bool test(int32_t val) const
    {
        return self().symbol() == val;
    }

    bool isNode() const
    {
        return test(1);
    }

    bool isLeaf() const
    {
        return test(0);
    }

    CtrSizeT nodeIdx() const
    {
        return self().gpos();
    }

    CtrSizeT countFw(int32_t symbol)
    {
        MEMORIA_V1_ASSERT_TRUE(symbol == 0 || symbol == 1);

        auto& self = this->self();

        return self.selectFw(1, 1 - symbol);
    }

    LoudsNode node() const
    {
        return LoudsNode(nodeIdx(), rank1(), value());
    }

    CtrSizeT nextSiblings()
    {
        return self().countFw(1);
    }

    CtrSizeT next_siblings() const
    {
        Iterator iter = self();
        return iter.countFw(1) - 1;
    }

    CtrSizeT prev_siblings() const
    {
        Iterator iter = self();
        return iter.countBw(1);
    }

    bool next_sibling()
    {
        auto& self = this->self();

        if (self++)
        {
            if (self.symbol() == 1)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else {
            throw Exception(MA_SRC, "Invalid tree structure");
        }
    }

    bool prev_sibling()
    {
        auto& self = this->self();

        if (self--)
        {
            if (self.symbol() == 1)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else {
            return false;
        }
    }

    void insertDegree(CtrSizeT degree)
    {
        auto& self = this->self();

        for (CtrSizeT c = 0; c < degree; c++)
        {
            self.insert(1);
        }

        self.insert(0);
    }

    void insertZeroes(CtrSizeT length)
    {
        auto& self = this->self();

        for (int64_t c = 0; c < length; c++)
        {
            self.insert(0);
        }
    }


    CtrSizeT select1Fw(CtrSizeT rank)
    {
        return self().selectFw(rank, 1);
    }

    CtrSizeT select1Bw(CtrSizeT rank)
    {
        return self().selectBw(rank, 1);
    }

    CtrSizeT rank1(CtrSizeT length) const
    {
        auto iter = self();
        return iter.rank(length, 1);
    }

    CtrSizeT rank1() const
    {
        return noderank_();
    }

    CtrSizeT rank0() const
    {
        int32_t nodeIdx = this->nodeIdx();
        int32_t rank1   = this->rank1();

        return nodeIdx + 1 - rank1;
    }

    void firstChild()
    {
        auto& self = this->self();

        CtrSizeT rank0 = self.rank0();

        self.selectFw(self.rank1() - rank0, 0);
        self++;
    }

    void lastChild()
    {
        auto& self = this->self();

        CtrSizeT rank0 = self.rank0();

        self.selectFw(self.rank1() + 1 - rank0, 0);
        self--;
    }

    void parent()
    {
        auto& self = this->self();

        self = self.ctr().parent(self.node());
    }

//    void check() const {
//      auto& self = this->self();
//
//      int64_t gpos     = self.gpos();
//      int64_t pos      = self.pos();
//
//      MEMORIA_V1_ASSERT(gpos, ==, pos);
//
//      int64_t rank1_a  = self.rank(1);
//      int64_t rank1_b  = self.iter_cache().rank1();
//
//      if (rank1_a != rank1_b)
//      {
//          cout<<"Check: "<<rank1_a<<" "<<rank1_b<<" "<<self.pos()<<endl;
//      }
//
//      MEMORIA_V1_ASSERT(rank1_a, ==, rank1_b);
//    }

    int32_t label_idx() const
    {
        auto& self = this->self();
        return self.label_idx(self.iter_local_pos());
    }

    int32_t label_idx(int32_t node_idx) const
    {
        auto& self = this->self();
        return self.localrank_(node_idx, 1);
    }



    LabelsTuple labels() const
    {
        auto& self = this->self();
        return self.ctr().getLabels(self.iter_leaf(), self.label_idx());
    }




    void insertNode(const LabelsTuple& tuple)
    {
        auto& self = this->self();

        self.ctr().insertNode(self, tuple);
        self.firstChild();
        self.ctr().insertZero(self);
    }

    void insertZero()
    {
        auto& self = this->self();
        self.ctr().insertZero(self);
    }


    template <int32_t LabelIdx>
    struct SumLabelFn {
        CtrSizeT sum_ = 0;

        using LeafPath = IntList<1, 1, LabelIdx>;

        template <int32_t Idx, typename Stream>
        void stream(const Stream* obj, int32_t block, int32_t idx)
        {
            if (obj != nullptr)
            {
                sum_ += obj->sum(block, 0, idx);
            }
        }

        template <typename NodeTypes>
        void treeNode(const BranchNode<NodeTypes>* node, WalkCmd, int32_t start, int32_t idx)
        {
            using BNode = BranchNode<NodeTypes>;
            using BranchPath = typename BNode::template BuildBranchPath<LeafPath>;

            int32_t block = BNode::template translateLeafIndexToBranchIndex<LeafPath>(0);

            node->template processStream<BranchPath>(*this, block, idx);
        }

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, WalkCmd, int32_t start, int32_t idx)
        {
            node->template processStream<LeafPath>(*this, 0, idx);
        }
    };

    template <int32_t LabelIdx>
    CtrSizeT sumLabel() const
    {
        auto& self = this->self();

        SumLabelFn<LabelIdx> fn;

        if (self.iter_local_pos() >= 0)
        {
            self.ctr().ctr_walk_tree_up(self.iter_leaf(), self.label_idx(), fn);
        }

        return fn.sum_;
    }

    template <int32_t LabelIdx, typename T>
    void setLabel(T&& value)
    {
        auto& self = this->self();
        self.ctr().template setLabel<LabelIdx>(self, std::forward<T>(value));
    }

    template <int32_t LabelIdx, typename T>
    void addLabel(T&& value)
    {
        auto& self = this->self();
        self.ctr().template addLabel<LabelIdx>(self, std::forward<T>(value));
    }

    void remove() {
        auto& self = this->self();
        self.ctr().remove(self);
    }


    auto skip_for_rank(CtrSizeT& r0, CtrSizeT r1)
    {
        auto& self = this->self();

        auto sym = self.value();

        if (sym) {
            r1++;
        }
        else {
            r0++;
        }

        return self.iter_skip_fw(1);
    }

    auto raw_rank(CtrSizeT& r0, CtrSizeT& r1, CtrSizeT idx)
    {
        auto& self = this->self();

        while (idx > 0 && !self.iter_is_end())
        {
            auto sym = self.value();

            if (sym)
            {
                r1++;
            }
            else {
                r0++;
            }

            idx--;

            self.iter_skip_fw(1);
        }
    }

    auto raw_select(int32_t symbol, CtrSizeT rank)
    {
        auto& self = this->self();
        CtrSizeT cnt = 0;

        while (!self.iter_is_end())
        {
            auto sym = self.value();

            if (sym == symbol)
            {
                rank--;

                if (!rank) {
                    break;
                }
            }

            cnt++;
            self.iter_skip_fw(1);
        }

        return cnt;
    }

    struct GPosFn {
        CtrSizeT pos_ = 0;

        GPosFn()  {}

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, WalkCmd, int32_t start, int32_t idx)
        {}

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, int32_t idx)
        {}

        template <typename NodeTypes>
        void treeNode(const BranchNode<NodeTypes>* node, WalkCmd, int32_t start, int32_t idx)
        {
            if (node != nullptr)
            {
                node->sum(0, 0, 0, idx, pos_);
            }
        }
    };


    CtrSizeT gpos() const
    {
        auto& self = this->self();

        GPosFn fn;

        self.ctr().ctr_walk_tree_up(self.iter_leaf(), self.iter_local_pos(), fn);

        return fn.pos_ + self.iter_local_pos();
    }


MEMORIA_V1_ITERATOR_PART_END


}}
