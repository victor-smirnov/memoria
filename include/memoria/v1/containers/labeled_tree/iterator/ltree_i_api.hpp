
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/containers/labeled_tree/ltree_names.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_tools.hpp>

#include <memoria/v1/core/container/iterator.hpp>

#include <memoria/v1/core/packed/wrappers/louds_tree.hpp>

#include <vector>
#include <iostream>

namespace memoria {
namespace v1 {

MEMORIA_V1_ITERATOR_PART_BEGIN(v1::louds::ItrApiName)
public:
    typedef Ctr<typename Types::CtrTypes>                                       Container;
    typedef Ctr<typename Types::IterTypes>                                      Iterator;

    typedef typename Container::Allocator                                       Allocator;
    typedef typename Container::NodeBaseG                                       NodeBaseG;

    typedef typename Container::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;
    typedef typename Container::Types::LabelsTuple                              LabelsTuple;

    typedef typename Container::Types::CtrSizeT                                 CtrSizeT;

    bool operator++() {
        return self().skipFw(1);
    }

    bool operator--() {
        return self().skipBw(1);
    }

    bool next() {
        return self().skipFw(1);
    }

    bool prev() {
        return self().skipBw(1);
    }


    bool operator++(int) {
        return self().skipFw(1);
    }

    bool operator--(int) {
        return self().skipFw(1);
    }

    CtrSizeT operator+=(CtrSizeT size) {
        return self().skipFw(size);
    }

    CtrSizeT operator-=(CtrSizeT size) {
        return self().skipBw(size);
    }

    Int size() const
    {
        return self().leafSize(0);
    }

    bool isEof() const {
        return self().idx() >= self().size();
    }

    bool isBof() const {
        return self().idx() < 0;
    }

    CtrSizeT skipFw(CtrSizeT amount) {
        return self().template skip_fw_<0>(amount);
    }

    CtrSizeT skipBw(CtrSizeT amount) {
        return self().template skip_bw_<0>(amount);
    }

    CtrSizeT skip(CtrSizeT amount) {
        return self().template skip_<0>(amount);
    }

    CtrSizeT pos() const
    {
        auto& self = this->self();

        return self.idx() + self.cache().size_prefix()[0];
    }

    CtrSizeT noderank_() const
    {
        auto& self = this->self();
//      return self.cache().rank1() + (self.symbol() == 1);

        return self.ranki(1);
    }

    bool isRoot() const
    {
        return self().pos() == 0;
    }

    Int value() const
    {
        if (!self().isEof())
        {
            return self().symbol();
        }
        else {
            return 0;
        }
    }

    bool test(Int val) const
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

    CtrSizeT countFw(Int symbol)
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

        for (BigInt c = 0; c < length; c++)
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
        Int nodeIdx = this->nodeIdx();
        Int rank1   = this->rank1();

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
//      BigInt gpos     = self.gpos();
//      BigInt pos      = self.pos();
//
//      MEMORIA_V1_ASSERT(gpos, ==, pos);
//
//      BigInt rank1_a  = self.rank(1);
//      BigInt rank1_b  = self.cache().rank1();
//
//      if (rank1_a != rank1_b)
//      {
//          cout<<"Check: "<<rank1_a<<" "<<rank1_b<<" "<<self.pos()<<endl;
//      }
//
//      MEMORIA_V1_ASSERT(rank1_a, ==, rank1_b);
//    }

    Int label_idx() const
    {
        auto& self = this->self();
        return self.label_idx(self.idx());
    }

    Int label_idx(Int node_idx) const
    {
        auto& self = this->self();
        return self.localrank_(node_idx, 1);
    }



    LabelsTuple labels() const
    {
        auto& self = this->self();
        return self.ctr().getLabels(self.leaf(), self.label_idx());
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


    template <Int LabelIdx>
    struct SumLabelFn {
        CtrSizeT sum_ = 0;

        using LeafPath = IntList<1, 1, LabelIdx>;

        template <Int Idx, typename Stream>
        void stream(const Stream* obj, Int block, Int idx)
        {
            if (obj != nullptr)
            {
                sum_ += obj->sum(block, 0, idx);
            }
        }

        template <typename NodeTypes>
        void treeNode(const BranchNode<NodeTypes>* node, WalkCmd, Int start, Int idx)
        {
            using BNode = BranchNode<NodeTypes>;
            using BranchPath = typename BNode::template BuildBranchPath<LeafPath>;

            Int block = BNode::template translateLeafIndexToBranchIndex<LeafPath>(0);

            node->template processStream<BranchPath>(*this, block, idx);
        }

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, WalkCmd, Int start, Int idx)
        {
            node->template processStream<LeafPath>(*this, 0, idx);
        }
    };

    template <Int LabelIdx>
    CtrSizeT sumLabel() const
    {
        auto& self = this->self();

        SumLabelFn<LabelIdx> fn;

        if (self.idx() >= 0)
        {
            self.ctr().walkUp(self.leaf(), self.label_idx(), fn);
        }

        return fn.sum_;
    }

    template <Int LabelIdx, typename T>
    void setLabel(T&& value)
    {
        auto& self = this->self();
        self.ctr().template setLabel<LabelIdx>(self, std::forward<T>(value));
    }

    template <Int LabelIdx, typename T>
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

        return self.skipFw(1);
    }

    auto raw_rank(CtrSizeT& r0, CtrSizeT& r1, CtrSizeT idx)
    {
        auto& self = this->self();

        while (idx > 0 && !self.isEnd())
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

            self.skipFw(1);
        }
    }

    auto raw_select(Int symbol, CtrSizeT rank)
    {
        auto& self = this->self();
        CtrSizeT cnt = 0;

        while (!self.isEnd())
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
            self.skipFw(1);
        }

        return cnt;
    }

    struct GPosFn {
        CtrSizeT pos_ = 0;

        GPosFn()  {}

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, WalkCmd, Int start, Int idx)
        {}

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, Int idx)
        {}

        template <typename NodeTypes>
        void treeNode(const BranchNode<NodeTypes>* node, WalkCmd, Int start, Int idx)
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

        self.ctr().walkUp(self.leaf(), self.idx(), fn);

        return fn.pos_ + self.idx();
    }


MEMORIA_V1_ITERATOR_PART_END


}}
