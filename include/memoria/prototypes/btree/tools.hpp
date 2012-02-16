
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTREE_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_TOOLS_HPP

#include <memoria/core/tools/fixed_vector.hpp>

namespace memoria    	{
namespace btree 		{

struct NodeRoomDescr {
	Int left_;
	Int right_;

	NodeRoomDescr(): left_(0), right_(0) {}
	NodeRoomDescr(Int left, Int right): left_(left), right_(right) {}
};



template <typename Node>
class TreePathItem {
	typedef TreePathItem<Node> MyType;

	Node node_;
	Int parent_idx_;
public:
	TreePathItem() {}
	TreePathItem(const MyType& other): node_(other.node_), parent_idx_(other.parent_idx_) {}
	TreePathItem(MyType&& other): node_(std::move(other.node_)), parent_idx_(other.parent_idx_) {}

	MyType& operator=(const MyType& other)
	{
		node_		= other.node_;
		parent_idx_	= other.parent_idx_;

		return *this;
	}

	MyType& operator=(MyType&& other)
	{
		node_		= std::move(other.node_);
		parent_idx_	= other.parent_idx_;

		return *this;
	}

	const Node& node() const {
		return node_;
	}

	Node& node() {
		return node_;
	}

	const Int& parent_idx() const {
		return parent_idx_;
	}

	Int& parent_idx() {
		return parent_idx_;
	}

	void Clear() {
		node_ 		= NULL;
		parent_idx_ = 0;
	}
};

template <typename Node>
class NodePair {
	typedef NodePair<Node> 		MyType;
	typedef TreePathItem<Node> 	Item;


	TreePathItem<Node> left_;
	TreePathItem<Node> right_;

public:
	NodePair() {}
	NodePair(const MyType& other): left_(other.left_), right_(other.right_) {}
	NodePair(MyType&& other): left_(std::move(other.left_)), right_(std::move(other.right_)) {}

	MyType& operator=(const MyType& other)
	{
		left_		= other.left_;
		right_		= other.right_;

		return *this;
	}

	MyType& operator=(MyType&& other)
	{
		left_		= other.left_;
		right_		= other.right_;

		return *this;
	}

	const Item& left() const
	{
		return left_;
	}

	Item& left()
	{
		return left_;
	}

	const Item& right() const
	{
		return right_;
	}

	Item& right()
	{
		return right_;
	}

	void Clear()
	{
		left_.Clear();
		right_.Clear();
	}
};


struct ValueClearing {
	template <typename Value>
	void operator()(Value& value)
	{
		value.Clear();
	}
};


template <typename Key, typename Counters, Int Indexes>
class Accumulators
{
	typedef Accumulators<Key, Counters, Indexes> MyType;

	Counters 	counters_;
	Key 		keys_[Indexes];

public:

	Accumulators()
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = 0;
		}
	}

	Accumulators(const MyType& other): counters_(other.counters_)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = other.keys_[c];
		}
	}

	const Key* keys() const {
		return keys_;
	}

	Key* keys() {
		return keys_;
	}

	const Counters& counters() const {
		return counters_;
	}

	Counters& counters() {
		return counters_;
	}

	bool operator==(const MyType& other) const
	{
		for (Int c = 0; c < Indexes; c++)
		{
			if (keys_[c] != other.keys_[c]) {
				return false;
			}
		}

		return counters_ == other.counters_;
	}

	MyType& operator=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = other.keys_[c];
		}

		counters_ = other.counters_;

		return *this;
	}

	MyType& operator+=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] += other.keys_[c];
		}

		counters_ += other.counters_;

		return *this;
	}

	MyType operator+(const MyType& other) const
	{
		Counters result = this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] += other.keys_[c];
		}

		result.counters_ += other.counters_;

		return result;
	}

	MyType operator-(const MyType& other) const
	{
		Counters result = this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] -= other.keys_[c];
		}

		result.counters_ -= other.counters_;

		return result;
	}

	MyType operator-() const
	{
		Counters result = this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] = -keys_[c];
		}

		result.counters_ = -counters_;

		return result;
	}


	MyType& operator-=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] += other.keys_[c];
		}

		counters_ += other.counters_;

		return *this;
	}



};

}
}


#endif

