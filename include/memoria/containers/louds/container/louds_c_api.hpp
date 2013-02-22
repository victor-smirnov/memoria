
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

namespace memoria    {

using namespace memoria::btree;

using namespace louds;

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

	Iterator parent(const LoudsNode& node)
	{
		BigInt rank 	= me()->rank0(node.node());
		Iterator iter 	= me()->select1(rank);

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

		iter.iter().skipFw(child_num);

		iter.node_rank() += child_num;

		return iter;
	}

	LoudsNode childNode(const LoudsNode& node, BigInt child_num)
	{
		return child(node, child_num).node();
	}


	bool isLeaf(const LoudsNode& node)
	{
		auto iter = me()->ctr().seek(node.node());
		return iter.test(0);
	}

	Iterator firstChild(const LoudsNode& node)
	{
		BigInt rank 	= me()->rank1(node.node());
		Iterator iter 	= me()->select0(rank);

		iter.skipFw();

		return iter;
	}

	Iterator lastChild(const LoudsNode& node)
	{
		BigInt rank 	= me()->rank1(node.node()) + 1;
		Iterator iter 	= me()->select0(rank);

		iter--;

		return iter;
	}

MEMORIA_CONTAINER_PART_END

}


#endif
