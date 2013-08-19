
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

	typedef Ctr<typename Types::CtrTypes>                      					Container;
	typedef Ctr<typename Types::IterTypes>                      				Iterator;

	typedef typename Container::Allocator                                       Allocator;
	typedef typename Container::NodeBaseG                                       NodeBaseG;

	typedef typename Container::Accumulator                               		Accumulator;
	typedef typename Container::LeafDispatcher                                	LeafDispatcher;
	typedef typename Container::Position										Position;
	typedef typename Container::Types::LabelsTuple								LabelsTuple;

    BigInt node_rank() const
	{
		auto& self = this->self();
//		return self.cache().rank1() + (self.symbol() == 1);

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

    BigInt nodeIdx() const
    {
    	return self().pos();
    }

    BigInt countFw(Int symbol)
    {
    	MEMORIA_ASSERT_TRUE(symbol == 0 || symbol == 1);

    	auto& self = this->self();

    	return self.selectFw(1, 1 - symbol);
    }

    LoudsNode node() const
    {
    	return LoudsNode(nodeIdx(), rank1(), value());
    }

    BigInt nextSiblings()
    {
    	return self().countFw(1);
    }

    BigInt next_siblings() const
    {
    	Iterator iter = self();
    	return iter.countFw(1) - 1;
    }

    BigInt prev_siblings() const
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

    void insertDegree(BigInt degree)
    {
    	auto& self = this->self();

    	for (BigInt c = 0; c < degree; c++)
    	{
    		self.insert(1);
    	}

    	self.insert(0);
    }

    void insertZeroes(BigInt length)
    {
    	auto& self = this->self();

    	for (BigInt c = 0; c < length; c++)
    	{
    		self.insert(0);
    	}
    }


    BigInt select1Fw(BigInt rank)
    {
    	return self().selectFw(rank, 1);
    }

    BigInt select1Bw(BigInt rank)
    {
    	return self().selectBw(rank, 1);
    }

    BigInt rank1(BigInt length) const
    {
    	auto iter = self();
    	return iter.rank(length, 1);
    }

    BigInt rank1() const
    {
    	return node_rank();
    }

    BigInt rank0() const
    {
    	Int nodeIdx = this->nodeIdx();
    	Int rank1 	= this->rank1();

    	return nodeIdx + 1 - rank1;
    }

    void firstChild()
    {
    	auto& self = this->self();

    	BigInt rank0 = self.rank0();

    	self.selectFw(self.rank1() - rank0, 0);
    	self++;
    }

    void lastChild()
    {
    	auto& self = this->self();

    	BigInt rank0 = self.rank0();

    	self.selectFw(self.rank1() + 1 - rank0, 0);
    	self--;
    }

    void parent()
    {
    	auto& self = this->self();

    	self = self.ctr().parent(self.node());
    }

//    void check() const {
//    	auto& self = this->self();
//
//		BigInt gpos  	= self.gpos();
//		BigInt pos 		= self.pos();
//
//		MEMORIA_ASSERT(gpos, ==, pos);
//
//		BigInt rank1_a 	= self.rank(1);
//		BigInt rank1_b	= self.cache().rank1();
//
//		if (rank1_a != rank1_b)
//		{
//			cout<<"Check: "<<rank1_a<<" "<<rank1_b<<" "<<self.pos()<<endl;
//		}
//
//		MEMORIA_ASSERT(rank1_a, ==, rank1_b);
//    }

    Int label_idx() const
    {
    	auto& self = this->self();
    	return self.label_idx(self.idx());
    }

    Int label_idx(Int node_idx) const
    {
    	auto& self = this->self();
    	return self.local_rank(node_idx, 1);
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
		BigInt sum_ = 0;
		Int block_ = 0;


		template <Int Idx, typename StreamTypes>
		void stream(const PkdVTree<StreamTypes>* obj, Int idx)
		{
			if (obj != nullptr)
			{
				sum_ += obj->sum(block_, 0, idx);
			}

			block_ = 1;
		}

		template <Int Idx, typename StreamTypes>
		void stream(const PkdFTree<StreamTypes>* obj, Int idx)
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
    BigInt sumLabel() const
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
//    	return IDataAdapter<WrappedIterator>(*me()->iter(), length);
//    }

MEMORIA_ITERATOR_PART_END


}


#endif
