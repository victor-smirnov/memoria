
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKERS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/container/iterator.hpp>

#include <memoria/prototypes/balanced_tree/nodes/tree_node.hpp>
#include <memoria/prototypes/balanced_tree/nodes/leaf_node.hpp>

#include <ostream>
#include <functional>


namespace memoria       {
namespace balanced_tree {

template <typename Types>
class SkipWalkerBase {
protected:
	typedef typename Types::Position 											Position;
	typedef typename Types::Key 												Key;

	typedef Iter<typename Types::IterTypes> 									Iterator;

	static const Int Streams													= Types::Streams;

	Position prefix_;

	BigInt sum_ = 0;

	BigInt target_;

	WalkDirection direction_;

	Int stream_;
	Int block_;
	Int size_indexes_[Streams];

public:

	SkipWalkerBase(Int stream, Int block, BigInt target):
		target_(target),
		stream_(stream),
		block_(block)
	{
		for (auto& i: size_indexes_) i = 0;
	}

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

	BigInt sum() const {
		return sum_;
	}

	Int stream() const {
		return stream_;
	}

	const Int& size_indexes(Int idx) const {
		return size_indexes_[idx];
	}

	Int& size_indexes(Int idx) {
		return size_indexes_[idx];
	}

	Position& prefix() {
		return prefix_;
	}

	const Position& prefix() const {
		return prefix_;
	}

	void prepare(Iterator& iter)
	{
		prefix_ = iter.cache().sizePrefix();
	}


};





template <typename Types>
class SkipForwardWalkerBase: public SkipWalkerBase<Types> {

protected:
	typedef SkipWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;
	typedef typename Base::Position 	Position;
	typedef typename Base::Iterator		Iterator;

	Int leafs_ = 0;

public:
	SkipForwardWalkerBase(Int stream, Int block, Key target): Base(stream, block, target)
	{}

	typedef Int ReturnType;
	typedef Int ResultType;

	template <typename Node>
	ResultType treeNode(const Node* node, Int start)
	{
		Int idx = node->find(Base::stream_, *this, start);

		for (Int s = 0; s < Base::Streams; s++)
		{
			if (s != Base::stream_)
			{
				node->sum(s, Base::size_indexes_[s], start, idx, Base::prefix_[s]);
			}
		}

		return idx;
	}

	template <Int Idx, typename Tree>
	ResultType stream(const Tree* tree, Int start)
	{
		auto k 		= Base::target_ - Base::sum_;
		auto result = tree->findLTForward(Base::size_indexes_[Idx], start, k);

		Base::sum_ += result.prefix();

		Base::prefix_[Base::stream_] += result.prefix();

		return result.idx();
	}
};






template <typename Types>
class SkipForwardWalker: public SkipForwardWalkerBase<Types> {

	typedef SkipForwardWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;
	typedef typename Base::Position 	Position;
	typedef typename Base::Iterator		Iterator;

	Int leafs_ = 0;

public:
	SkipForwardWalker(Int stream, Int block, Key target): Base(stream, block, target)
	{}

	Int leafs() const {
		return leafs_;
	}

	typedef Int ReturnType;
	typedef Int ResultType;


	template <typename Node>
	ResultType treeNode(const Node* node, Int start)
	{
		return Base::template treeNode(node, start);
	}


	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int start)
	{
		leafs_++;

		auto& sum = Base::sum_;

		BigInt offset  = Base::target_ - sum;

		Position sizes = node->sizes();
		Int 	 size  = sizes[Base::stream_];

		if (start + offset < size)
		{
			sum += start + offset;
			return start + offset;
		}
		else {
			sum 			+= (size - start);
			Base::prefix_ 	+= sizes;

			return size;
		}
	}



	BigInt finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = idx;

		Int size = iter.leafSize(Base::stream_);

		if (idx < size) {
			iter.cache().setSizePrefix(Base::prefix_);
		}
		else {
			iter.cache().setSizePrefix(Base::prefix_ - Position(size));
		}

		return Base::sum_;
	}
};




template <typename Types>
class NextLeafWalker: public SkipForwardWalkerBase<Types> {

	typedef SkipForwardWalkerBase<Types> 		Base;
	typedef typename Base::Key 					Key;
	typedef typename Base::Position 			Position;
	typedef typename Base::Iterator				Iterator;

	Int leafs_ = 0;

public:

	typedef Int ReturnType;
	typedef Int ResultType;


	NextLeafWalker(Int stream, Int block): Base(stream, block, 0)
	{}

	Int leafs() const {
		return leafs_;
	}



	template <typename Node>
	ResultType treeNode(const Node* node, Int start)
	{
		return Base::template treeNode(node, start);
	}


	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int start)
	{
		leafs_++;

		if (leafs_ == 1)
		{
			Position sizes 	= node->sizes();
			Base::prefix_ 	+= sizes;

			return sizes[Base::stream_];
		}
		else {
			return 0;
		}
	}

	bool finish(Iterator& iter, Int idx)
	{
		Int size = iter.leafSizes(Base::stream_);

		if (idx < size)
		{
			iter.cache().setSizePrefix(Base::prefix_);
			iter.key_idx() = 0;

			return true;
		}
		else {
			iter.cache().setSizePrefix(Base::prefix_ - Position(size));
			iter.key_idx() = size;

			return false;
		}
	}
};






template <typename Types>
class NextLeafMultistreamWalker: public SkipWalkerBase<Types> {

	typedef SkipForwardWalkerBase<Types> 		Base;
	typedef typename Base::Key 					Key;
	typedef typename Base::Position 			Position;
	typedef typename Base::Iterator				Iterator;

	static const Int Streams					= Types::Streams;

	Int leafs_ = 0;

	UBigInt 	streams_;
	Position 	local_prefixes_;
	Int 		indexes_[Streams];


public:

	typedef Int ReturnType;
	typedef Int ResultType;


	NextLeafMultistreamWalker(UBigInt streams, Int block):
		Base(0, block, 0),
		streams_(streams)
	{}

	Int leafs() const {
		return leafs_;
	}

	template <typename Node>
	ResultType treeNode(const Node* node, Int start)
	{
		local_prefixes_.clear();
		for (auto& i: indexes_) i = -1;

		node->processNotEmpty(streams_, *this, start);

		Int min 	= 1<<31;
		Int min_idx = Streams;

		for (Int c = 0; c < Streams; c++)
		{
			Int index = indexes_[c];
			if (Base::isEnabled(c) && index < min)
			{
				min 	= index;
				min_idx = c;
			}
		}

		MEMORIA_ASSERT_TRUE(min_idx < Streams);
		MEMORIA_ASSERT_TRUE(min_idx >= 0);

		for (Int s = 0; s < Base::Streams; s++)
		{
			if (s != min_idx)
			{
				BigInt prefix = 0;

				node->sum(s, Base::size_indexes_[s], start, min_idx, prefix);

				Base::prefix_[s] 	+= prefix;
				Base::sums_[s] 		+= prefix;
			}
			else {
				Base::prefix_[s] 	+= local_prefixes_[s];
				Base::sums_[s] 		+= local_prefixes_[s];
			}
		}

		return min_idx;
	}

	template <Int Idx, typename Tree>
	void stream(const Tree* tree, Int start)
	{
		auto k 		= Base::target_ - Base::sum_;
		auto result = tree->findLTForward(Base::size_indexes_[Idx], start, k);

		local_prefixes_[Idx] = result.prefix();
		indexes_[Idx]		 = result.idx();
	}


	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int start)
	{
		leafs_++;

		if (leafs_ == 1)
		{
			Position sizes 	= node->sizes();
			Base::prefix_ 	+= sizes;

			return sizes[Base::stream_];
		}
		else {
			return 0;
		}
	}

	bool finish(Iterator& iter, Int idx)
	{
		Int size = iter.leafSizes(Base::stream_);

		if (idx < size)
		{
			iter.cache().setSizePrefix(Base::prefix_);
			iter.key_idx() = 0;

			return true;
		}
		else {
			iter.cache().setSizePrefix(Base::prefix_ - Position(size));
			iter.key_idx() = size;

			return false;
		}
	}
};









template <typename Types>
class SkipBackwardWalkerBase: public SkipWalkerBase<Types> {
protected:
	typedef SkipWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;
	typedef typename Base::Position 	Position;
	typedef typename Base::Iterator		Iterator;

public:
	SkipBackwardWalkerBase(Int stream, Int block, Key target): Base(stream, block, target)
	{}

	typedef Int ReturnType;
	typedef Int ResultType;

	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		Int idx = node->find(Base::stream_, *this, start);

		for (Int s = 0; s < Base::Streams; s++)
		{
			if (s != Base::stream_)
			{
				node->sum(s, Base::size_indexes_[s], idx + 1, start, Base::prefix_[s]);
			}
		}

		return idx;
	}

	template <Int Idx, typename Tree>
	ResultType stream(const Tree* tree, Int start)
	{
		auto k 			= Base::target_ - Base::sum_;
		auto result 	= tree->findLTBackward(Base::size_indexes_[Idx], start, k);
		Base::sum_ 		+= result.prefix();

		Base::prefix_[Base::stream_] -= result.prefix();

		return result.idx();
	}
};





template <typename Types>
class SkipBackwardWalker: public SkipBackwardWalkerBase<Types> {

	typedef SkipBackwardWalkerBase<Types> 		Base;
	typedef typename Base::Key 					Key;
	typedef typename Base::Position 			Position;
	typedef typename Base::Iterator				Iterator;

	Int leafs_ = 0;

public:
	SkipBackwardWalker(Int stream, Int block, Key target): Base(stream, block, target)
	{}

	typedef Int ReturnType;
	typedef Int ResultType;

	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		return Base::template treeNode(node, start);
	}

	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		leafs_++;

		BigInt offset = Base::target_ - Base::sum_;

		auto& sum = Base::sum_;

		Position sizes = node->sizes();

		if (leafs_ == 2)
		{
			Base::prefix_ -= sizes;
		}

		if (start - offset >= 0)
		{
			sum += offset;
			return start - offset;
		}
		else {
			sum += start;
			return -1;
		}
	}

	BigInt finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = idx;
		iter.cache().setSizePrefix(Base::prefix_);

		return Base::sum_;
	}
};




template <typename Types>
class PrevLeafWalker: public SkipBackwardWalkerBase<Types> {

	typedef SkipBackwardWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;
	typedef typename Base::Position 	Position;
	typedef typename Base::Iterator		Iterator;

	Int leafs_ = 0;

public:
	PrevLeafWalker(Int stream, Int block): Base(stream, block, 0)
	{}

	typedef Int ReturnType;
	typedef Int ResultType;

	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		return Base::template treeNode(node, start);
	}

	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		leafs_++;

		Position sizes = node->sizes();

		if (leafs_ == 2)
		{
			Base::prefix_ -= sizes;
			return sizes[Base::stream_] > 0;
		}
		else {
			return -1;
		}
	}

	bool finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = idx;
		iter.cache().setSizePrefix(Base::prefix_);

		return idx >= 0;
	}
};





template <typename Types>
class PrevLeafMultistreamWalker: public SkipWalkerBase<Types> {

	typedef SkipBackwardWalkerBase<Types> 		Base;
	typedef typename Base::Key 					Key;
	typedef typename Base::Position 			Position;
	typedef typename Base::Iterator				Iterator;

	static const Int Streams					= Types::Streams;

	Int leafs_ = 0;

	UBigInt 	streams_;
	Position 	local_prefixes_;
	Int 		indexes_[Streams];


public:
	PrevLeafMultistreamWalker(UBigInt streams, Int block):
		Base(stream, block, 0),
		streams_(streams)
	{}

	typedef Int ReturnType;
	typedef Int ResultType;

	bool isEnabled(Int stream) const
	{
		return streams_ & (1ull << stream);
	}

	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		local_prefixes_.clear();
		for (auto& i: indexes_) i = -1;

		node->processNotEmpty(streams_, *this, start);

		Int max 	= -1;
		Int max_idx = -1;

		for (Int c = 0; c < Streams; c++)
		{
			Int index = indexes_[c];
			if (Base::isEnabled(c) && index > max)
			{
				max 	= index;
				max_idx = c;
			}
		}

		MEMORIA_ASSERT_TRUE(max_idx < Streams);
		MEMORIA_ASSERT_TRUE(max_idx >= 0);

		for (Int s = 0; s < Base::Streams; s++)
		{
			if (s != max_idx)
			{
				BigInt prefix = 0;
				node->sum(s, Base::size_indexes_[s], max_idx + 1, start, prefix);

				Base::prefix_[s] 	-= prefix;
				Base::sums_[s] 		+= prefix;
			}
			else {
				Base::prefix_[s] 	-= local_prefixes_[s];
				Base::sums_[s] 		+= local_prefixes_[s];
			}
		}

		return max_idx;
	}

	template <Int Idx, typename Tree>
	void stream(const Tree* tree, Int start)
	{
		auto k 			= Base::target_ - Base::sum_;
		auto result 	= tree->findLTBackward(Base::size_indexes_[Idx], start, k);

		local_prefixes_[Idx] = result.prefix();
		indexes_[Idx]		 = result.idx();
	}

	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		leafs_++;

		Position sizes = node->sizes();

		if (leafs_ == 2)
		{
			Base::prefix_ -= sizes;
			return sizes[Base::stream_] > 0;
		}
		else {
			return -1;
		}
	}

	bool finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = idx;
		iter.cache().setSizePrefix(Base::prefix_);

		return idx >= 0;
	}
};






}
}

#endif

