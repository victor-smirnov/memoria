
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTORMAP_CONTAINER_WALKERS_HPP
#define _MEMORIA_CONTAINERS_VECTORMAP_CONTAINER_WALKERS_HPP

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/balanced_tree/baltree_walkers.hpp>

#include <memoria/prototypes/balanced_tree/nodes/leaf_node.hpp>

#include <ostream>

namespace memoria       {
namespace vmap      	{


template <typename Types>
class FindWalkerBase {
protected:
	typedef typename Types::Key 												Key;
	typedef typename Types::Accumulator 										Accumulator;

	typedef Iter<typename Types::IterTypes> 									Iterator;

	BigInt prefix_ = 0;

	Key key_;

	WalkDirection direction_;

public:

	FindWalkerBase(Key key):
		key_(key)
	{}

	const WalkDirection& direction() const {
		return direction_;
	}

	WalkDirection& direction() {
		return direction_;
	}


	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() 	= idx;

		Int size = iter.size();

		if (idx < size)
		{
			iter.cache().setup(prefix_);
		}
		else {
			iter.cache().setup(prefix_ - size);
		}
	}

	void empty(Iterator& iter)
	{
		iter.key_idx()	= 0;

		iter.cache().setup(0);
	}

	BigInt prefix() const {
		return prefix_;
	}
};





template <typename Types>
class FindLTForwardWalker: public FindWalkerBase<Types> {

	typedef FindWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;

	Int leafs_ = 0;

public:
	FindLTForwardWalker(Key key, Int): Base(key)
	{}

	Int leafs() const {
		return leafs_;
	}

	typedef Int ReturnType;
	typedef Int ResultType;


	template <typename Node>
	ResultType treeNode(const Node* node, Int start)
	{
		return node->find(0, *this, start);
	}

	template <Int Idx, typename Tree>
	ResultType stream(const Tree* tree, Int start)
	{
		auto k 		= Base::key_ - Base::prefix_;
		auto result = tree->findLTForward(0, start, k);

		Base::prefix_ 	+= result.prefix();

		return result.idx();
	}



	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int start)
	{
		leafs_++;

		BigInt pos = Base::key_ - Base::prefix_;

		Int size = node->size(0);

		if (start + pos < size)
		{
			return start + pos;
		}
		else {
			Base::prefix_ += (size - start);
			return size;
		}
	}
};



template <typename Types>
class FindLTBackwardWalker: public FindWalkerBase<Types> {

	typedef FindWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;

public:
	FindLTBackwardWalker(Key key, Int): Base(key)
	{}

	typedef Int ReturnType;
	typedef Int ResultType;

	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		return node->find(0, *this, start);
	}

	template <Int Idx, typename Tree>
	ResultType stream(const Tree* tree, Int start)
	{
		auto k 			= Base::key_ - Base::prefix_;
		auto result 	= tree->findLTBackward(0, start, k);
		Base::prefix_ 	+= result.prefix();

		return result.idx();
	}


	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		BigInt pos = Base::key_ - Base::prefix_;

		Base::prefix_ += start;

		if (start - pos >= 0)
		{
			return start - pos;
		}
		else {
			return -1;
		}
	}
};








template <typename Types>
class FindLEWalker: public FindWalkerBase<Types> {

	typedef FindWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;

public:
	FindLEWalker(Key key, Int key_num): Base(key, key_num)
	{}

	template <typename Node>
	void treeNode(const Node* node)
	{
		Base::idx_ = node->findLES(Base::key_num_, Base::key_ - std::get<0>(Base::prefix_)[Base::key_num_], Base::prefix_);

		if (node->level() != 0 && Base::idx_ == node->children_count())
		{
			VectorSub(Base::prefix_, node->keysAt(node->children_count() - 1));
			Base::idx_--;
		}
	}
};







template <typename Types>
class FindRangeWalkerBase {
protected:
	typedef Iter<typename Types::IterTypes> Iterator;
	typedef Ctr<typename Types::CtrTypes> 	Container;

	typedef typename Types::Accumulator		Accumulator;

	WalkDirection direction_;

public:
	FindRangeWalkerBase() {}

	WalkDirection& direction() {
		return direction_;
	}

	void empty(Iterator& iter)
	{
		iter.cache().setup(Accumulator());
	}
};



template <typename Types>
class FindEndWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

	typedef typename Types::Accumulator 	Accumulator;

	Accumulator prefix_;

public:
	typedef Int ReturnType;

	FindEndWalker(Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node)
	{
		if (node->level() > 0)
		{
			VectorAdd(prefix_, node->maxKeys() - node->keysAt(node->children_count() - 1));
		}
		else {
			VectorAdd(prefix_, node->maxKeys());
		}

		return node->children_count() - 1;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx + 1;
		iter.cache().setup(prefix_);
	}
};


template <typename Types>
class FindREndWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;
	typedef typename Types::Accumulator 	Accumulator;

public:
	typedef Int ReturnType;

	FindREndWalker(Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node)
	{
		return 0;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx - 1;

		iter.cache().setup(Accumulator());
	}
};



template <typename Types>
class FindBeginWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;
	typedef typename Types::Accumulator 	Accumulator;

public:
	typedef Int ReturnType;


	FindBeginWalker(Container&) {}


	template <typename Node>
	ReturnType treeNode(const Node* node)
	{
		return 0;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = 0;

		iter.cache().setup(Accumulator());
	}
};

template <typename Types>
class FindRBeginWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

	typedef typename Types::Accumulator 	Accumulator;

	Accumulator prefix_;

public:
	FindRBeginWalker(Container&) {}

	typedef Int ReturnType;



	template <typename Node>
	ReturnType treeNode(const Node* node)
	{
		VectorAdd(prefix_, node->maxKeys() - node->keysAt(node->children_count() - 1));

		return node->children_count() - 1;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.cache().setup(prefix_);
	}
};


}
}

#endif
