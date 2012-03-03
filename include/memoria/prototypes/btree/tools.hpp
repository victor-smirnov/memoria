
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTREE_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_TOOLS_HPP

#include <memoria/core/tools/fixed_vector.hpp>

namespace memoria    	{
namespace btree 		{


template <typename Node>
class TreePathItem {
	typedef TreePathItem<Node> MyType;
	typedef typename Node::Page Page;

	Node node_;
	Int parent_idx_;
public:

	TreePathItem(): node_(), parent_idx_(0) {}

	TreePathItem(Node& node, Int parent_idx = 0): node_(node), parent_idx_(parent_idx) {}

	TreePathItem(const MyType& other): node_(other.node_), parent_idx_(other.parent_idx_) {}
	TreePathItem(MyType&& other): node_(std::move(other.node_)), parent_idx_(other.parent_idx_) {}

	Page* operator->()
	{
		return node_.operator->();
	}

	const Page* operator->() const
	{
		return node_.operator->();
	}


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

	operator Node& () {
		return node_;
	}

	operator const Node& () const {
		return node_;
	}

	operator Node () const {
		return node_;
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


template <typename Key, Int Indexes>
class Accumulators
{
	typedef Accumulators<Key, Indexes> MyType;

	Key 		keys_[Indexes];

public:

	Accumulators()
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = 0;
		}
	}

	Accumulators(const MyType& other)
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

	bool operator==(const MyType& other) const
	{
		for (Int c = 0; c < Indexes; c++)
		{
			if (keys_[c] != other.keys_[c]) {
				return false;
			}
		}

		return true;
	}

	MyType& operator=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] = other.keys_[c];
		}

		return *this;
	}

	MyType& operator+=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] += other.keys_[c];
		}

		return *this;
	}

	MyType operator+(const MyType& other) const
	{
		MyType result = *this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] += other.keys_[c];
		}

		return result;
	}

	MyType operator-(const MyType& other) const
	{
		MyType result = *this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] -= other.keys_[c];
		}

		return result;
	}

	MyType operator-() const
	{
		MyType result = *this;

		for (Int c = 0; c < Indexes; c++)
		{
			result.keys_[c] = -keys_[c];
		}

		return result;
	}


	MyType& operator-=(const MyType& other)
	{
		for (Int c = 0; c < Indexes; c++)
		{
			keys_[c] += other.keys_[c];
		}

		return *this;
	}
};

}
}


#endif

