
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

#include <memoria/core/packed/wrappers/louds_tree.hpp>

#include <functional>

namespace memoria    {

using namespace louds;
using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrApiName)

	typedef typename Base::Iterator 											Iterator;



	LoudsNode rootNode()
	{
		return self().seek(0).node();
	}

	Iterator parent(const LoudsNode& node)
	{
		auto iter = self().select1(node.rank0());
		return iter;
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

		iter().skipFw(child_num);

		iter.node_rank() += child_num;

		return iter;
	}

	LoudsNode childNode(const LoudsNode& node, BigInt child_num)
	{
		return child(node, child_num).node();
	}

	Iterator firstChild(const LoudsNode& node)
	{
		Iterator iter = self().select0(node.rank1());

		iter++;

		return iter;
	}

	Iterator lastChild(const LoudsNode& node)
	{
		Iterator iter 	= self().select0(node.rank1() + 1);

		iter--;

		return iter;
	}

	BigInt nodes()
	{
		auto& self = this->self();

		return self.rank1(self.size() - 1);
	}

//	LoudsTree getLoudsSubtree(const LoudsNode& node)
//	{
//		BigInt tree_size = 0;
//
//		this->traverseSubtree(node, [&tree_size](const Iterator& left, BigInt length, Int level) {
//			tree_size += length;
//		});
//
//		LoudsTree tree(tree_size);
//
//		this->traverseSubtree(node, [&tree, this](const Iterator& left, BigInt length, Int level) {
//			if (length >= 0)
//			{
//				if (length > 0)
//				{
//					auto src = left.source(length - 1);
//					tree.append(src);
//				}
//
//				tree.appendUDS(0);
//			}
//		});
//
//		tree.reindex();
//
//		return tree;
//	}

	BigInt getSubtreeSize(const LoudsNode& node)
	{
		BigInt count = 0;

		this->traverseSubtree(node, [&count](const Iterator& left, BigInt length, Int level) {
//			cout<<"TS: "<<left.pos()<<" "<<left.cpos()<<" "<<length<<" "<<level<<endl;

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



	void traverseSubtree(const LoudsNode& node, function<void (const Iterator&, BigInt, Int)> fn)
	{
		Iterator left = self().seek(node.node());
		Iterator right = left;

		traverseSubtree(left, right, fn);
	}

private:
	void traverseSubtree(Iterator& left, Iterator& right, function<void (const Iterator&, BigInt, Int)> fn, Int level = 0)
	{
		BigInt length = right.nodeIdx() - left.nodeIdx() + 1;

		fn(left, length + 1, level);

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
