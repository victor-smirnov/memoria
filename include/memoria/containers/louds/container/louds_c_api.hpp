
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LOUDS_LOUDS_C_API_HPP
#define MEMORIA_CONTAINERS_LOUDS_LOUDS_C_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/louds/louds_names.hpp>
#include <memoria/containers/louds/louds_tools.hpp>

#include <memoria/prototypes/ctr_wrapper/iterator.hpp>

#include <functional>

namespace memoria    {

using namespace memoria::btree;
using namespace louds;
using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrApiName)

	typedef typename Base::Iterator 											Iterator;

	Iterator begin() {
		return Iterator(*me(), me()->ctr().begin());
	}

	Iterator Begin() {
		return begin();
	}

	Iterator End() {
		return Iterator(*me(), me()->ctr().End());
	}

	Iterator end()
	{
		Iterator iter(*me());
		iter.type() = Iterator::END;
		return iter;
	}

	IterEndMark endm()
	{
		return IterEndMark();
	}

	LoudsNode rootNode()
	{
		return me()->find(0).node();
	}

	Iterator parent(const LoudsNode& node)
	{
		return me()->select1(node.rank0());
	}

	LoudsNode parentNode(const LoudsNode& node)
	{
		return parent(node).node();
	}


	LoudsNodeRange children(const LoudsNode& node)
	{
		Iterator iter = firstChild(node);

		LoudsNode first_child = iter.node();

		BigInt count = iter.nextSiblings();

		return LoudsNodeRange(first_child, count);
	}

	Iterator child(const LoudsNode& node, BigInt child_num)
	{
		Iterator iter = firstChild(node);

		iter.iter().skipFw(child_num);

		iter.node_rank() += child_num;

		return iter;
	}

	LoudsNode childNode(const LoudsNode& node, BigInt child_num)
	{
		return child(node, child_num).node();
	}

	Iterator firstChild(const LoudsNode& node)
	{
		Iterator iter 	= me()->select0(node.rank1());

		iter.skipFw();

		return iter;
	}

	Iterator lastChild(const LoudsNode& node)
	{
		Iterator iter 	= me()->select0(node.rank1() + 1);

		iter--;

		return iter;
	}

	BigInt size() {
		return me()->ctr().size();
	}

	BigInt nodes() {
		return me()->rank1(size() - 1);
	}

	BigInt getSubtreeSize(const LoudsNode& node)
	{
		BigInt count = 0;

		this->traverseSubtree(node, [&count](const Iterator& left, BigInt length, Int level) {
			if (level == 0)
			{
				count += left.rank1(length - 1);
			}
			else {
				count += left.rank1(length);
			}
		});

		return count;
	}


//	template <typename Functor>
	void traverseSubtree(const LoudsNode& node, function<void (const Iterator&, BigInt, Int)> fn)
	{
		Iterator left = me()->find(node.node());
		Iterator right = left;

		traverseSubtree(left, right, fn);
	}

private:

//	template <typename Functor>
	void traverseSubtree(Iterator& left, Iterator& right, function<void (const Iterator&, BigInt, Int)> fn, Int level = 0)
	{
		fn(left, right.nodeIdx() - left.nodeIdx() + 1, level);

		bool left_leaf 		= left.isLeaf();
		bool right_leaf		= right.isLeaf();

		if (!left_leaf || !right_leaf || !is_end(left, right))
		{
			if (left_leaf)
			{
				left.select1Fw(1);
			}

			if (right_leaf)
			{
				right.select1Bw(1);
			}

			if (!left.isEof())
			{
				left.firstChild();
				right.lastChild();

				traverseSubtree(left, right, fn, level + 1);
			}
		}
	}

	bool is_end(Iterator& left, Iterator& right)
	{
		return left.nodeIdx() >= right.nodeIdx();
	}

MEMORIA_CONTAINER_PART_END

}


#endif
