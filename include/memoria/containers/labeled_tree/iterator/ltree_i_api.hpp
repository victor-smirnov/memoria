
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CONTAINERS_LBLTREE_ITERATOR_API_HPP
#define MEMORIA_CONTAINERS_LBLTREE_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/containers/labeled_tree/ltree_names.hpp>
#include <memoria/containers/labeled_tree/ltree_tools.hpp>

#include <memoria/core/container/iterator.hpp>

#include <memoria/core/packed/wrappers/louds_tree.hpp>

#include <vector>
#include <iostream>

namespace memoria    {

MEMORIA_ITERATOR_PART_BEGIN(memoria::louds::ItrApiName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;
    typedef Ctr<typename Types::IterTypes>                                      Iterator;

    typedef typename Container::Allocator                                       Allocator;
    typedef typename Container::NodeBaseG                                       NodeBaseG;

    typedef typename Container::Accumulator                                     Accumulator;
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
        return self().pos();
    }

    CtrSizeT countFw(Int symbol)
    {
        MEMORIA_ASSERT_TRUE(symbol == 0 || symbol == 1);

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
//      MEMORIA_ASSERT(gpos, ==, pos);
//
//      BigInt rank1_a  = self.rank(1);
//      BigInt rank1_b  = self.cache().rank1();
//
//      if (rank1_a != rank1_b)
//      {
//          cout<<"Check: "<<rank1_a<<" "<<rank1_b<<" "<<self.pos()<<endl;
//      }
//
//      MEMORIA_ASSERT(rank1_a, ==, rank1_b);
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
        Int block_ = 0;


        template <Int Idx, typename StreamTypes>
        void stream(const PkdVQTree<StreamTypes>* obj, Int idx)
        {
            if (obj != nullptr)
            {
                sum_ += obj->sum(block_, 0, idx);
            }

            block_ = 1;
        }

        template <Int Idx, typename StreamTypes>
        void stream(const PkdFQTree<StreamTypes>* obj, Int idx)
        {
            if (obj != nullptr)
            {
                sum_ += obj->sum(block_, 0, idx);
            }

            block_ = 1;
        }

        template <typename Node>
        void treeNode(const Node* node, Int idx)
        {
            node->template processStream<LabelIdx + 1>(*this, idx);
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

    void setLabel(Int label, BigInt value)
    {
        auto& self = this->self();
        self.ctr().setLabel(self, label, value);
    }

    void addLabel(Int label, BigInt value)
    {
        auto& self = this->self();
        self.ctr().addLabel(self, label, value);
    }


//    IDataAdapter<WrappedIterator> source(BigInt length = -1) const
//    {
//      return IDataAdapter<WrappedIterator>(*me()->iter(), length);
//    }


    void remove() {
    	auto& self = this->self();
    	self.ctr().remove(self);
    }

MEMORIA_ITERATOR_PART_END


}


#endif
