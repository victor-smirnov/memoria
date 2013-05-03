
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

	Accumulator prefix_;

	Key sum_ = 0;

	Int stream_;
	Int index_;
	Key key_;


	WalkDirection direction_;

public:

	FindWalkerBase(Int stream, Int index, Key key):
		stream_(stream),
		index_(index),
		key_(key)
	{}

	const WalkDirection& direction() const {
		return direction_;
	}

	WalkDirection& direction() {
		return direction_;
	}

	void prepare(Iterator& iter)
	{
		std::get<0>(prefix_)[0] = iter.cache().id_prefix();
		std::get<0>(prefix_)[1] = iter.cache().blob_base();
	}

	BigInt finish(Iterator& iter, Int idx)
	{
		iter.key_idx() 	= idx;

		BigInt id_prefix 	= std::get<0>(prefix_)[0];
		BigInt base 		= std::get<0>(prefix_)[1];

		BigInt id_entry 	= 0;
		BigInt size 		= 0;


		if (idx >=0 && idx < iter.leafSize(stream_))
		{
			if (stream_ == 0)
			{
				auto entry	= iter.entry();

				id_entry 	= entry.first;
				size		= entry.second;
			}
		}

		iter.cache().setup(id_prefix, id_entry, size, base);

		return sum_;
	}


	void empty(Iterator& iter)
	{
		iter.key_idx()	= 0;
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
	FindLTForwardWalker(Int stream, Int index, Key key):
		Base(stream, index, key)
	{}

	Int leafs() const {
		return leafs_;
	}

	typedef Int ReturnType;
	typedef Int ResultType;

	template <typename Node>
	ResultType treeNode(const Node* node, Int start)
	{
		return node->find(Base::stream_, *this, start);
	}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start)
	{
		typedef PackedFSETree<TreeTypes> Tree;

		auto& index = Base::index_;

		auto k 		= Base::key_ - Base::sum_;
		auto result = tree->findLTForward(index, start, k);

		Base::sum_	+= result.prefix();

		if (Idx == 0)
		{
			std::get<Idx>(Base::prefix_)[index] 		+= result.prefix();
			std::get<Idx>(Base::prefix_)[1 - index] 	+= tree->sum(index, start, result.idx());
		}

		return result.idx();
	}


	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSEArray<TreeTypes>* tree, Int start)
	{
		BigInt pos = Base::key_ - Base::sum_;

		Int size = tree->size();

		if (start + pos < size)
		{
			return start + pos;
		}
		else {
			return size;
		}
	}



	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int start)
	{
		leafs_++;

		return node->find(Base::stream_, *this, start);
	}
};



template <typename Types>
class FindLTBackwardWalker: public FindWalkerBase<Types> {

	typedef FindWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;

	Int leafs_ = 0;

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


	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start)
	{
		auto k 			= Base::key_ - Base::sum_;
		auto result 	= tree->findLTBackward(0, start, k);
		Base::sum_ 		+= result.prefix();

		if (Idx == 0)
		{

		}

		return result.idx();
	}


	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSEArray<TreeTypes>* tree, Int start)
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


	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		leafs_++;
		return node->find(Base::stream_, *this, start);
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
	}
};





template <typename Types>
class FindVMapEndWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 											Base;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;

	typedef typename Types::Accumulator 										Accumulator;


	Accumulator 	prefix_;
	Accumulator 	local_prefix_;
	Int size_ 		= 0;

	Int stream_;

public:
	typedef Int ReturnType;

	FindVMapEndWalker(Int stream, Container&):
		stream_(stream)
	{}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		node->process(stream_, *this, node->level(), start);
		return size_ - 1;
	}

	template <Int Idx, typename TreeTypes>
	void stream(const PackedFSEArray<TreeTypes>* tree, Int level, Int start)
	{
		MEMORIA_INVALID_STREAM(Idx);
	}

	template <Int StreamIdx, typename TreeTypes>
	void stream(const PackedFSETree<TreeTypes>* tree, Int level, Int start)
	{
		typedef PackedFSETree<TreeTypes> Tree;

		if (level > 0)
		{
			for (Int block = 0; block < Tree::Blocks; block++)
			{
				std::get<StreamIdx>(local_prefix_)[block] = tree->sumWithoutLastElement(block);
			}

			std::get<StreamIdx>(prefix_) += std::get<StreamIdx>(local_prefix_);
		}
		else {
			for (Int block = 0; block < Tree::Blocks; block++)
			{
				std::get<StreamIdx>(local_prefix_)[block] = tree->sum(block);
			}

			std::get<StreamIdx>(prefix_) += std::get<StreamIdx>(local_prefix_);
		}

		size_ = tree->size();
	}


	void finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = idx;

		iter.found() = false;

		BigInt id_prefix = std::get<0>(prefix_)[0];
		BigInt base		 = std::get<0>(prefix_)[1];

		iter.cache().setup(id_prefix, 0, base, 0);
	}
};


template <typename Types>
class FindVMapBeginWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

	typedef typename Types::Accumulator 	Accumulator;


public:
	typedef Int ReturnType;

	FindVMapBeginWalker(Int stream, Container&)
	{}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		return 0;
	}


	void finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = 0;
	}
};


}
}

#endif
