
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

#include <vector>
#include <iostream>

namespace memoria    {

using namespace std;
using namespace ::memoria::louds;

MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::louds::ItrApiName)


	BigInt node_rank_;

	IterPart(): Base(), node_rank_(0) {}
    IterPart(ThisPartType&& other): Base(std::move(other)), node_rank_(other.node_rank_) {}
    IterPart(const ThisPartType& other): Base(other), node_rank_(other.node_rank_) {}
    virtual ~IterPart() throw() {}

    const BigInt& node_rank() const {
    	return node_rank_;
    }

    BigInt& node_rank() {
    	return node_rank_;
    }

    Int value() const
    {
    	if (!me()->iter().isEof())
    	{
    		return me()->iter().element();
    	}
    	else {
    		return 0;
    	}
    }

    bool test(Int value) const
    {
    	return me()->iter().test(value);
    }

    bool isNode() const
    {
    	return test(1);
    }

    bool isLeaf() const
    {
    	return test(0);
    }

    bool isEof() const {
    	return me()->iter().isEof();
    }

    bool operator++()
    {
    	if (me()->iter()++)
    	{
    		node_rank_ += test(1);
    		return true;
    	}

    	return false;
    }

    bool skipFw() {
    	if (me()->iter()++)
    	{
    		node_rank_ += test(1);
    		return true;
    	}

    	return false;
    }

    bool operator--()
    {
    	if (me()->iter()--)
    	{
    		node_rank_ -= test(1);
    		return true;
    	}

    	return false;
    }

    BigInt nodeIdx() const
    {
    	return me()->iter().pos();
    }

    LoudsNode node() const
    {
    	return LoudsNode(nodeIdx(), node_rank(), value());
    }

    BigInt nextSiblings()
    {
    	BigInt count = me()->iter().countFw(1);

    	node_rank_ += count;

    	return count;
    }

    void insertDegree(BigInt degree)
    {
    	node_rank_ += degree;

    	NodeDegreeSourceAdaptor<typename MyType::WrappedIterator::ElementType> adaptor(degree);

    	me()->iter().insert(adaptor);
    }

    void insertZeroes(BigInt length)
    {
    	ZeroSourceAdaptor<typename MyType::WrappedIterator::ElementType> adaptor(length);

    	me()->iter().insert(adaptor);
    }

    void dump(ostream& out = cout)
    {
    	me()->iter().dump(out);
    }

    BigInt select1Fw(BigInt rank)
    {
    	BigInt actual_rank = me()->iter().selectFw(rank, 1);
    	node_rank_ += actual_rank;
    	return actual_rank;
    }

    BigInt select1Bw(BigInt rank)
    {
    	BigInt actual_rank = me()->iter().selectBw(rank, 1);
    	node_rank_ -= actual_rank;
    	return actual_rank;
    }

    BigInt rank1(BigInt length) const
    {
    	return me()->iter().rank(length, 1);
    }

    void firstChild() {

    }

    void lastChild() {

    }

MEMORIA_ITERATOR_PART_END


}


#endif
