
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



template <typename Types, typename MyType>
class FindWalkerBase {
protected:
	typedef typename Types::Position 											Position;
	typedef typename Types::Key 												Key;

	typedef Iter<typename Types::IterTypes> 									Iterator;

	static const Int Streams													= Types::Streams;

	BigInt sum_ 		= 0;
	BigInt target_		= 0;

	WalkDirection direction_;

	Int stream_;
	Int index_;

public:

	typedef Int 																ReturnType;

	FindWalkerBase(Int stream, Int index, BigInt target):
		target_(target),
		stream_(stream),
		index_(index)
	{}

	const WalkDirection& direction() const {
		return direction_;
	}

	WalkDirection& direction() {
		return direction_;
	}

	BigInt finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = idx;

		return sum_;
	}

	void empty(Iterator& iter)
	{
		iter.key_idx()	= 0;
	}

	BigInt sum() const {
		return sum_;
	}

	Int stream() const {
		return stream_;
	}

	void prepare(Iterator& iter) {}


	MyType& self()
	{
		return *T2T<MyType*>(this);
	}

	const MyType& self() const
	{
		return *T2T<const MyType*>(this);
	}

	template <Int StreamIdx>
	void postProcessStreamPrefix(BigInt) {}

	template <typename Node>
	void postProcessNode(const Node*, Int, Int) {}

	template <typename Node>
	ReturnType treeNode(const Node* node, BigInt start)
	{
		Int idx = node->find(stream_, self(), start);

		self().postProcessNode(node, start, idx);

		return idx;
	}
};



template <typename Types, typename MyType>
class FindMinWalkerBase {
protected:
	typedef typename Types::Position 											Position;
	typedef typename Types::Key 												Key;

	typedef Iter<typename Types::IterTypes> 									Iterator;

	static const Int Streams													= Types::Streams;

	BigInt sum_[Streams];
	BigInt target_[Streams];

	WalkDirection direction_;

	UBigInt streams_;
	Int indexes_[Streams];

	Int search_results_[Streams];

public:

	typedef Int 																ReturnType;
	typedef Int 																ResultType;

	FindMinWalkerBase(UBigInt streams):
		streams_(streams)
	{
		for (auto& i: sum_) 	i = 0;
		for (auto& i: target_) 	i = 0;
		for (auto& i: indexes_)	i = 0;
	}

	const WalkDirection& direction() const {
		return direction_;
	}

	WalkDirection& direction() {
		return direction_;
	}

	BigInt sum(Int stream) const {
		return sum_[stream];
	}

	const BigInt& target(Int stream) const {
		return target_[stream];
	}

	BigInt& target(Int stream) {
		return target_[stream];
	}

	const Int& indexes(Int stream) const {
		return indexes_[stream];
	}

	Int& indexes(Int stream) {
		return indexes_[stream];
	}

	BigInt finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = idx;

		return sum_;
	}

	void empty(Iterator& iter)
	{
		iter.key_idx()	= 0;
	}

	void prepare(Iterator& iter) {}


	MyType& self()
	{
		return *T2T<MyType*>(this);
	}

	const MyType& self() const
	{
		return *T2T<const MyType*>(this);
	}

	template <Int StreamIdx>
	void postProcessStreamPrefix(BigInt) {}

	template <typename Node>
	void postProcessNode(const Node*, Int, Int) {}

	bool isEnabled(Int stream) const
	{
		return streams_ & (1ull << stream);
	}
};





template <typename Types, typename MyType>
class FindForwardWalkerBase: public FindWalkerBase<Types, MyType> {

protected:
	typedef FindWalkerBase<Types, MyType> 										Base;
	typedef typename Base::Key 													Key;

	Int leafs_ = 0;

public:
	FindForwardWalkerBase(Int stream, Int block, Key target): Base(stream, block, target)
	{}

	typedef Int ResultType;

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start)
	{
		auto k 		= Base::target_ - Base::sum_;
		auto result = tree->findLTForward(Base::index_, start, k);

		Base::sum_ += result.prefix();

		self().template postProcessStreamPrefix<Idx>(result.prefix());

		return result.idx();
	}

	template <Int Idx, typename StreamTypes>
	ResultType stream(const PackedFSEArray<StreamTypes>* array, Int start)
	{
		leafs_++;

		auto& sum = Base::sum_;

		BigInt offset = Base::target_ - sum;

		Int	size = array->size();

		if (start + offset < size)
		{
			sum += start + offset;

			return start + offset;
		}
		else {
			sum += (size - start);

			return size;
		}
	}

	MyType& self() {
		return *T2T<MyType*>(this);
	}

	const MyType& self() const {
		return *T2T<const MyType*>(this);
	}
};


template <typename Types>
class SkipForwardWalker: public FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {
	typedef FindForwardWalkerBase<Types, SkipForwardWalker<Types>> 				Base;
	typedef typename Base::Key 													Key;
public:
	SkipForwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{}
};











template <typename Types, typename MyType>
class NextLeafWalkerBase: public FindForwardWalkerBase<Types, MyType> {

	typedef FindForwardWalkerBase<Types, MyType> 								Base;
	typedef typename Base::Key 													Key;
	typedef typename Base::Position 											Position;
	typedef typename Base::Iterator												Iterator;

	Int leafs_ = 0;

public:

	typedef Int ReturnType;
	typedef Int ResultType;

	NextLeafWalkerBase(Int stream, Int index): Base(stream, index, 0)
	{}

	Int leafs() const {
		return leafs_;
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
			iter.key_idx() = 0;

			return true;
		}
		else {
			iter.key_idx() = size;

			return false;
		}
	}
};



template <typename Types>
class NextLeafWalker: public NextLeafWalkerBase<Types, NextLeafWalker<Types> > {

	typedef NextLeafWalkerBase<Types, NextLeafWalker<Types>> 					Base;
	typedef typename Base::Key 													Key;
	typedef typename Base::Position 											Position;
	typedef typename Base::Iterator												Iterator;

public:

	NextLeafWalker(Int stream, Int block): Base(stream, block, 0)
	{}
};






template <typename Types, typename MyType>
class FindMinForwardWalkerBase: public FindMinWalkerBase<Types, MyType> {

	typedef FindMinWalkerBase<Types, MyType> 									Base;
	typedef typename Base::Key 													Key;
	typedef typename Base::Position 											Position;
	typedef typename Base::Iterator												Iterator;

protected:
	static const Int Streams													= Types::Streams;

	Int leafs_ = 0;

public:

	typedef Int ReturnType;
	typedef Int ResultType;


	FindMinForwardWalkerBase(UBigInt streams):
		Base(streams)
	{}

	Int leafs() const {
		return leafs_;
	}

	template <typename Node>
	ResultType treeNode(const Node* node, Int start)
	{
		for (auto& i: Base::search_results_) i = -1;

		node->processNotEmpty(Base::streams_, self(), start);

		Int min 	= 1<<31;
		Int min_idx = Streams;

		for (Int c = 0; c < Streams; c++)
		{
			Int index = Base::search_results_[c];
			if (Base::isEnabled(c) && index < min)
			{
				min 	= index;
				min_idx = c;
			}
		}

		MEMORIA_ASSERT_TRUE(min_idx < Streams);
		MEMORIA_ASSERT_TRUE(min_idx >= 0);

		self().postProcessNode(node, start, min_idx);

		return min_idx;
	}


	template <Int Idx, typename Tree>
	void stream(const Tree* tree, Int start)
	{
		auto k 		= Base::target_[Idx] - Base::sum_[Idx];
		auto result = tree->findLTForward(Base::indexes_[Idx], start, k);

		Base::sum_[Idx] 			+= result.prefix();
		Base::search_results_[Idx]	= result.idx();
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
			iter.key_idx() = 0;

			return true;
		}
		else {
			iter.key_idx() = size;

			return false;
		}
	}


	MyType& self() {
		return *T2T<MyType*>(this);
	}

	const MyType& self() const {
		return *T2T<const MyType*>(this);
	}
};



template <typename Types>
class NextLeafMultistreamWalker: public FindMinForwardWalkerBase<Types, NextLeafMultistreamWalker<Types> > {
	typedef FindMinForwardWalkerBase<Types, NextLeafMultistreamWalker<Types>> 	Base;
public:
	NextLeafMultistreamWalker(UBigInt streams): Base(streams) {}
};






template <typename Types, typename MyType>
class FindBackwardWalkerBase: public FindWalkerBase<Types, MyType> {

	typedef FindWalkerBase<Types, MyType> 										Base;

protected:
	typedef typename Base::Key 													Key;

	Int leafs_ = 0;

public:
	typedef Int 																ResultType;

	FindBackwardWalkerBase(Int stream, Int index, Key target): Base(stream, index, target)
	{}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start)
	{
		auto k 			= Base::target_ - Base::sum_;
		auto result 	= tree->findLTBackward(Base::index_, start, k);
		Base::sum_ 		+= result.prefix();

		self().template postProcessStreamPrefix<Idx>(result.prefix());

		return result.idx();
	}


	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSEArray<TreeTypes>* array, Int start)
	{
		leafs_++;

		BigInt offset = Base::target_ - Base::sum_;

		auto& sum = Base::sum_;

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

	MyType& self() {
		return *T2T<MyType*>(this);
	}

	const MyType& self() const {
		return *T2T<const MyType*>(this);
	}
};


template <typename Types>
class SkipBackwardWalker: public FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {
	typedef FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>>			Base;
	typedef typename Base::Key 													Key;
public:
	SkipBackwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{}
};






template <typename Types, typename MyType>
class PrevLeafWalkerBase: public FindBackwardWalkerBase<Types, MyType> {

	typedef FindBackwardWalkerBase<Types, MyType> 								Base;
	typedef typename Base::Key 													Key;
	typedef typename Base::Position 											Position;
	typedef typename Base::Iterator												Iterator;

	Int leafs_ = 0;

public:
	PrevLeafWalkerBase(Int stream, Int index): Base(stream, index, 0)
	{}

	typedef Int ReturnType;
	typedef Int ResultType;

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
		return idx >= 0;
	}
};


template <typename Types>
class PrevLeafWalker: public PrevLeafWalkerBase<Types, PrevLeafWalker<Types>> {

	typedef PrevLeafWalkerBase<Types, PrevLeafWalker<Types>> 					Base;
	typedef typename Base::Key 													Key;
	typedef typename Base::Position 											Position;
	typedef typename Base::Iterator												Iterator;

public:
	PrevLeafWalker(Int stream, Int index): Base(stream, index, 0)
	{}
};





template <typename Types, typename MyType>
class FindMinBackwardWalker: public FindMinWalkerBase<Types, MyType> {

	typedef FindMinWalkerBase<Types, MyType> 									Base;
	typedef typename Base::Key 													Key;
	typedef typename Base::Position 											Position;
	typedef typename Base::Iterator												Iterator;

	static const Int Streams													= Types::Streams;

	Int leafs_ = 0;

public:
	FindMinBackwardWalker(UBigInt streams):
		Base(streams)
	{}

	typedef Int ReturnType;
	typedef Int ResultType;


	template <typename NodeTypes, bool root, bool leaf>
	ReturnType treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, BigInt start)
	{
		for (auto& i: Base::search_results_) i = -1;

		node->processNotEmpty(Base::streams_, self(), start);

		Int max 	= -1;
		Int max_idx = -1;

		for (Int c = 0; c < Streams; c++)
		{
			Int index = Base::search_results_[c];
			if (Base::isEnabled(c) && index > max)
			{
				max 	= index;
				max_idx = c;
			}
		}

		MEMORIA_ASSERT_TRUE(max_idx < Streams);
		MEMORIA_ASSERT_TRUE(max_idx >= 0);

		self().postProcessNode(node, start, max_idx);

		return max_idx;
	}

	template <Int Idx, typename Tree>
	void stream(const Tree* tree, Int start)
	{
		auto k 		= Base::target_[Idx] - Base::sum_[Idx];
		auto result = tree->findLTBackward(Base::indexes_[Idx], start, k);

		Base::sum_[Idx] 			+= result.prefix();
		Base::search_results_[Idx]	= result.idx();
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

		return idx >= 0;
	}

	MyType& self() {
		return *T2T<MyType*>(this);
	}

	const MyType& self() const {
		return *T2T<const MyType*>(this);
	}
};


template <typename Types>
class PrevLeafMultistreamWalker: public FindMinForwardWalkerBase<Types, PrevLeafMultistreamWalker<Types> > {
	typedef FindMinForwardWalkerBase<Types, PrevLeafMultistreamWalker<Types>> 	Base;
public:
	PrevLeafMultistreamWalker(UBigInt streams): Base(streams) {}
};




}
}

#endif

