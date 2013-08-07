
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CONTAINERS_LOUDS_ITERATOR_API_HPP
#define MEMORIA_CONTAINERS_LOUDS_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/containers/louds/louds_names.hpp>
#include <memoria/containers/louds/louds_tools.hpp>

#include <memoria/core/container/iterator.hpp>

#include <memoria/core/packed/wrappers/louds_tree.hpp>

#include <vector>
#include <iostream>

namespace memoria    {

MEMORIA_ITERATOR_PART_BEGIN(memoria::louds::ItrApiName)

	typedef Ctr<typename Types::CtrTypes>                      					Container;

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

    BigInt rank1() const {
    	return node_rank();
    }

    BigInt rank0() const {
    	return nodeIdx() + 1 - rank1();
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

    	self = self.parent(self.node());
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


//    IDataAdapter<WrappedIterator> source(BigInt length = -1) const
//    {
//    	return IDataAdapter<WrappedIterator>(*me()->iter(), length);
//    }

MEMORIA_ITERATOR_PART_END


}


#endif
