
// Copyright Victor Smirnov 2012-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_WALKERS_HPP

#include <memoria/prototypes/sequence/names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/tools/idata.hpp>

namespace memoria    {



namespace sequence {


template <typename Types>
class FindWalkerBase {
protected:
	typedef typename Types::Key 												Key;
	typedef Iter<typename Types::IterTypes> 									Iterator;

	Key key_;
	Int key_num_;

	Key prefix_;
	Int idx_;

	WalkDirection direction_;

	Int start_;

public:
	FindWalkerBase(Key key, Int key_num):
		key_(key), key_num_(key_num), prefix_(0), idx_(0)
	{}

	WalkDirection& direction() {
		return direction_;
	}

	const Int& start() const {
		return start_;
	}

	Int& start() {
		return start_;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.cache().setup(prefix_, 0);
	}

	void empty(Iterator& iter)
	{
		iter.cache().setup(0, 0);
	}

	Int idx() const {
		return idx_;
	}
};


template <typename Types>
class FindLTWalker: public FindWalkerBase<Types> {

	typedef FindWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;

public:
	FindLTWalker(Key key, Int key_num): Base(key, key_num)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		const typename Node::Map& map = node->map();

		Base::idx_ = map.findLTS(Base::key_num_, Base::key_ - Base::prefix_, Base::prefix_);

		if (node->level() != 0 && Base::idx_ == map.size())
		{
			Base::prefix_ -= map.key(0, map.size() - 1);
			Base::idx_--;
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
	void operator()(const Node* node)
	{
		const typename Node::Map& map = node->map();

		Base::idx_ = map.findLES(Base::key_num_, Base::key_ - Base::prefix_, Base::prefix_);

		if (node->level() != 0 && Base::idx_ == map.size())
		{
			Base::prefix_ -= map.key(0, map.size() - 1);
			Base::idx_--;
		}
	}
};


template <typename Types>
class FindRangeWalkerBase {
protected:
	typedef Iter<typename Types::IterTypes> Iterator;
	typedef Ctr<typename Types::CtrTypes> 	Container;

	Int idx_;

	WalkDirection direction_;

	Int start_;

public:
	FindRangeWalkerBase(): idx_(0) {}

	WalkDirection& direction() {
		return direction_;
	}

	const Int& start() const {
		return start_;
	}

	Int& start() {
		return start_;
	}

	void empty(Iterator& iter)
	{
		iter.cache().setup(0, 0);
	}

	Int idx() const
	{
		return idx_;
	}
};



template <typename Types>
class FindEndWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

	typedef typename Types::Key 			Key;

	Key prefix_;

public:
	FindEndWalker(Container& ctr): prefix_(0) {}

	template <typename Node>
	void operator()(const Node* node)
	{
		const typename Node::Map& map = node->map();

		Base::idx_ = node->children_count() - 1;

		prefix_ += map.maxKey(0) - map.key(0, map.size() - 1);
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.model().finishPathStep(iter.path(), iter.key_idx());
		iter.dataPos() = iter.data()->size();

		iter.cache().setup(prefix_, 0);
	}
};


template <typename Types>
class FindREndWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

public:
	FindREndWalker(Container&) {}

	template <typename Node>
	void operator()(const Node* node)
	{
		Base::idx_ = 0;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.model().finishPathStep(iter.path(), iter.key_idx());
		iter.dataPos() = -1;

		iter.cache().setup(0, 0);
	}
};




template <typename Types>
class FindBeginWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

public:
	FindBeginWalker(Container&) {}


	template <typename Node>
	void operator()(const Node* node)
	{
		Base::idx_ = 0;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = 0;

		iter.model().finishPathStep(iter.path(), iter.key_idx());
		iter.dataPos() = 0;

		iter.cache().setup(0, 0);
	}
};

template <typename Types>
class FindRBeginWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

	typedef typename Types::Key 			Key;

	Key prefix_;

public:
	FindRBeginWalker(Container&): prefix_(0) {}

	template <typename Node>
	void operator()(const Node* node)
	{
		const typename Node::Map& map = node->map();

		Base::idx_ = node->children_count() - 1;

		prefix_ += map.maxKey(0) - map.key(0, map.size() - 1);
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.model().finishPathStep(iter.path(), iter.key_idx());
		iter.dataPos() = iter.data()->size() - 1;

		iter.cache().setup(prefix_, 0);
	}
};


template <
	typename Types
>
class MultiPrefixWalkerBase {
protected:
	typedef typename Types::Key 												Key;
	typedef Iter<typename Types::IterTypes> 									Iterator;
	typedef Ctr<typename Types::CtrTypes> 										Container;

	Container& ctr_;

	Key key_;

	Int key_count_;
	const Int* key_nums_;

	Key* prefixes_;

	Int idx_;


	WalkDirection direction_;

	Int start_;

public:
	MultiPrefixWalkerBase(Container& ctr, Key key, Int key_count, const Int* key_nums, Key* prefixes):
		ctr_(ctr),
		key_(key),
		key_count_(key_count),
		key_nums_(key_nums),
		prefixes_(prefixes),
		idx_(0)
	{}

	WalkDirection& direction() {
		return direction_;
	}

	const Int& start() const {
		return start_;
	}

	Int& start() {
		return start_;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		ctr_.finishPathStep(iter.path(), iter.key_idx());
	}

	void empty(Iterator& iter) {}

	const Int& idx() const {
		return idx_;
	}

	Int& idx() {
		return idx_;
	}
};





template <
	typename Types
>
class SinglePrefixWalkerBase {
protected:
	typedef typename Types::Key 												Key;
	typedef Iter<typename Types::IterTypes> 									Iterator;
	typedef Ctr<typename Types::CtrTypes> 										Container;

	Container& ctr_;

	Int key_num_;

	BigInt prefix_;

	Int idx_;

	WalkDirection direction_;

	Int start_;

public:
	SinglePrefixWalkerBase(Container& ctr, Int key_num):
		ctr_(ctr),
		key_num_(key_num),
		prefix_(0),
		idx_(0)
	{}

	WalkDirection& direction() {
		return direction_;
	}

	const Int& start() const {
		return start_;
	}

	Int& start() {
		return start_;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		ctr_.finishPathStep(iter.path(), iter.key_idx());
	}

	void empty(Iterator& iter) {}

	const Int& idx() const {
		return idx_;
	}

	Int& idx() {
		return idx_;
	}

	BigInt prefix() const {
		return prefix_;
	}
};




template <
	typename Types
>
class SequenceSkipFWWalker: public SinglePrefixWalkerBase<Types> {
protected:
	typedef SinglePrefixWalkerBase<Types>										Base;
	typedef typename Types::Key 												Key;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;

	BigInt key_;

public:
	SequenceSkipFWWalker(Container& ctr, Int key_num, BigInt key):
		Base(ctr, key_num), key_(key)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		typedef typename Node::Map Map;

		const Map& map = node->map();

		Base::idx_ = map.findSumPositionFwLT(Base::key_num_, Base::start_, key_ - Base::prefix_, Base::prefix_);

		if (Base::idx_ == map.size() && Base::direction_ == WalkDirection::DOWN)
		{
			Base::prefix_ -= map.key(Base::key_num_, map.size() - 1);
			Base::idx_--;
		}
	}
};


template <
	typename Types
>
class SequenceSkipBWWalker: public SinglePrefixWalkerBase<Types> {
protected:
	typedef SinglePrefixWalkerBase<Types>										Base;
	typedef typename Types::Key 												Key;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;

	BigInt key_;

public:
	SequenceSkipBWWalker(Container& ctr, Int key_num, Int key):
		Base(ctr, key_num), key_(key)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		typedef typename Node::Map Map;

		const Map& map = node->map();

		Base::idx_ = map.findSumPositionBwLE(Base::key_num_, Base::start_, key_ - Base::prefix_, Base::prefix_);

		if (Base::idx_ == -1 && Base::direction_ == WalkDirection::DOWN)
		{
			Base::prefix_ -= map.key(Base::key_num_, 0);
			Base::idx_++;
		}
	}
};






}




}

#endif
