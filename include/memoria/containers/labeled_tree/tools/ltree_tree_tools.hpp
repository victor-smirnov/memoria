
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LBLTREE_TOOLS_TREE_HPP_
#define MEMORIA_CONTAINERS_LBLTREE_TOOLS_TREE_HPP_

#include <memoria/core/tools/isequencedata.hpp>
#include <memoria/core/types/types.hpp>

#include <utility>

namespace memoria 	{
namespace louds 	{

class LoudsNode {
protected:
	BigInt 	node_;
	BigInt 	rank1_;
	Int 	value_;

public:
	LoudsNode(BigInt node, BigInt rank1, Int value = -1):
		node_(node),
		rank1_(rank1),
		value_(value)
	{}

	LoudsNode():
		node_(0),
		rank1_(0),
		value_(0)
	{}

	BigInt node() const {return node_;}
	BigInt rank1() const {return rank1_;}
	BigInt rank0() const {return node_ + 1 - rank1_;}

	Int value() const {return value_;}

	bool isLeaf() const {
		return value_ == 0;
	}

	void operator++(int) {
		node_++;
		rank1_++;
	}

	bool operator<(const LoudsNode& other) const {
		return node_ < other.node_;
	}
};


class LoudsNodeRange: public LoudsNode {
	BigInt count_;
public:
	LoudsNodeRange(BigInt node, BigInt node_rank, Int value, BigInt count):
		LoudsNode(node, node_rank, value),
		count_(count)
	{}

	LoudsNodeRange(const LoudsNode& start, BigInt count):
		LoudsNode(start),
		count_(count)
	{}

	BigInt count() const {return count_;}

	LoudsNode first() const {
		return LoudsNode(node(), rank1(), value());
	}

	LoudsNode last() const {
		return LoudsNode(node() + count_, rank1() + count_, 0);
	}
};






}
}


#endif /* LBLTREE_TOOLS_HPP_ */