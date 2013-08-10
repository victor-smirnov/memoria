
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_VTREE_C_API_HPP
#define MEMORIA_CONTAINERS_VTREE_C_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/vector_tree/vtree_names.hpp>

#include <functional>

namespace memoria {


MEMORIA_CONTAINER_PART_BEGIN(memoria::vtree::CtrApiName)

	typedef typename Base::Tree													Tree;
	typedef typename Base::Vec													Vec;

	typedef typename Tree::Iterator												TreeIterator;
	typedef typename Vec::Iterator												VectorIterator;
	typedef typename Base::Iterator												Iterator;

	Iterator root_node()
	{
		auto& self = this->self();

		auto tree_iter 	= self.tree().seek(0);
		auto vec_iter	= self.vector().seek(0);

		return Iterator(self, tree_iter, vec_iter);
	}

	Iterator seek(BigInt node_idx)
	{
		auto& self = this->self();

		auto tree_iter 	 = self.tree().seek(0);
		BigInt data_base = tree_iter.template sumLabel<1>();
		auto vec_iter	 = self.vector().seek(data_base);

		return Iterator(self, tree_iter, vec_iter);
	}


	void insert(Iterator& pos, Int node_type)
	{
		self().tree().newNodeAt(pos.tree_iter(), std::make_tuple(node_type, 0));
	}

	void remove(Iterator& node)
	{
		self().tree().removeLeaf(node.tree_iter());
	}

	Iterator firstChild(Iterator& node)
	{
		auto& self = this->self();

		auto tree_iter 	 = self.tree().firstChild(node.tree_iter());
		BigInt data_base = tree_iter.template sumLabel<1>();
		auto vec_iter	 = self.vector().seek(data_base);

		return Iterator(self, tree_iter, vec_iter);
	}


	Iterator lastChild(Iterator& node)
	{
		auto& self = this->self();

		auto tree_iter 	 = self.tree().lastChild(node.tree_iter());
		BigInt data_base = tree_iter.template sumLabel<1>();
		auto vec_iter	 = self.vector().seek(data_base);

		return Iterator(self, tree_iter, vec_iter);
	}

MEMORIA_CONTAINER_PART_END

}


#endif
