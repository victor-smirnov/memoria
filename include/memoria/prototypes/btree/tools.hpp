
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTREE_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_TOOLS_HPP

#include <memoria/core/tools/fixed_vector.hpp>

#include <ostream>

namespace memoria    	{
namespace btree 		{

using namespace memoria::core;

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

	bool operator==(const MyType& other) const
	{
		return node_ == other.node_;
	}

	bool operator!=(const MyType& other) const
	{
		return node_ != other.node_;
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
		node_.Clear();
		parent_idx_ = 0;
	}
};


struct ValueClearing {
	template <typename Value>
	void operator()(Value& value)
	{
		value.Clear();
	}
};






template <
	typename NodePage,
	Int Size = 8>
class NodePath: public FixedVector<TreePathItem<NodePage>, Size, ValueClearing> {

	typedef FixedVector<TreePathItem<NodePage>, Size, ValueClearing> 	Base;
	typedef NodePath<NodePage, Size> 									MyType;

public:

	typedef TreePathItem<NodePage>										PathItem;

	NodePath(): Base() {}

	NodePath(const MyType& other): Base(other) {}

	NodePath(MyType&& other): Base(std::move(other)) {}

	MyType& operator=(const MyType& other)
	{
		Base::operator=(other);
		return *this;
	}

	MyType& operator=(MyType&& other)
	{
		Base::operator=(std::move(other));
		return *this;
	}

	void MoveRight(Int level, Int from, Int count)
	{
		if (level >= 0)
		{
			PathItem& item = Base::operator[](level);
			if (item.parent_idx() >= from)
			{
				item.parent_idx() += count;
			}
		}
	}

	void MoveLeft(Int level, Int from, Int count)
	{
		if (level >= 0)
		{
			PathItem& item = Base::operator[](level);

			if (item.parent_idx() >= from)
			{
				item.parent_idx() -= count;
			}
			else if (item.parent_idx() >= from)
			{
				for (Int c = level; c >= 0; c--)
				{
					Base::operator[](c).Clear();
				}
			}
		}
	}

	PathItem& leaf()
	{
		return this->operator[](0);
	}

	const PathItem& leaf() const
	{
		return this->operator[](0);
	}
};


template <typename Iterator>
class IteratorCacheBase {
	Iterator* iter_;
public:
	IteratorCacheBase() {}

	void init(Iterator* i)
	{
		iter_ = i;
	}

	void initState() {}


	void Prepare() 				{}
	void NextKey(bool end) 		{}
	void PrevKey(bool start) 	{}

	const Iterator& iterator() const {
		return *iter_;
	}

	Iterator& iterator() {
		return *iter_;
	}

	void setup(BigInt, Int) {}
};


template <typename Iterator, typename Container>
class BTreeIteratorCache: public IteratorCacheBase<Iterator> {
	typedef IteratorCacheBase<Iterator> Base;

	BigInt key_num_;
public:

	BTreeIteratorCache(): Base(), key_num_(0) {}

	BigInt& key_num()
	{
		return key_num_;
	}

	const BigInt& key_num() const
	{
		return key_num_;
	}
};



}
}

#endif

