
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAP2_CONTAINER_WALKERS_HPP
#define _MEMORIA_CONTAINERS_MAP2_CONTAINER_WALKERS_HPP

#include <memoria/prototypes/btree/tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/balanced_tree/baltree_walkers.hpp>

#include <ostream>

namespace memoria       {
namespace map2        	{


template <typename Types>
class FindWalkerBase {
protected:
	typedef typename Types::Key 												Key;
	typedef typename Types::Accumulator 										Accumulator;

	typedef Iter<typename Types::IterTypes> 									Iterator;

	Key prefix_ = 0;

	Key key_;
	Int key_num_;

	WalkDirection direction_;



public:
	FindWalkerBase(Key key, Int key_num):
		key_(key), key_num_(key_num)
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

		iter.cache().setup(prefix_);
	}

	void empty(Iterator& iter)
	{
		iter.key_idx()	= 0;

		iter.cache().setup(0);
	}
};


template <typename Types>
class FindLTWalker: public FindWalkerBase<Types> {

	typedef FindWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;

public:
	typedef Int ReturnType;

	FindLTWalker(Key key, Int key_num): Base(key, key_num)
	{}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		const typename Node::Map& map = node->map();

		return map.findLTS(Base::key_num_, Base::key_ - Base::prefix_[Base::key_num_], Base::prefix_);
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
		node->find(0, *this, node->level());
	}

	typedef Int ResultType;

	template <Int Idx, typename Tree>
	Int stream(const Tree* tree, Int level)
	{
		auto& key		= Base::key_;
		auto& prefix 	= Base::prefix_;
		auto& idx		= Base::idx_;

		auto target 	= key - prefix;

		auto result 	= tree->findLE(target);

		prefix += result.prefix();

		idx = result.idx();

		Int size = tree->size();

		if (level != 0 && idx == size)
		{
			prefix -= tree->value(size - 1);
			idx--;
		}

		return idx;
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
		iter.cache().setup(0);
	}
};



template <typename Types>
class FindEndWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

	typedef typename Types::Accumulator 	Accumulator;

	BigInt prefix_ = 0;

public:
	typedef Int ReturnType;

	FindEndWalker(Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		node->process(0, *this, node->level(), start);

		return node->children_count() - 1;
	}

	template <Int Idx, typename Tree>
	void stream(const Tree* tree, Int level, Int start)
	{
		if (level > 0)
		{
			prefix_ += tree->sumWithoutLastElement();
		}
		else {
			prefix_ += tree->sum();
		}
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
	ReturnType treeNode(const Node* node, Int start)
	{
		return 0;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx - 1;

		iter.cache().setup(0);
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
	ReturnType treeNode(const Node* node, Int start)
	{
		return 0;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = 0;

		iter.cache().setup(0);
	}
};

template <typename Types>
class FindRBeginWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

	typedef typename Types::Accumulator 	Accumulator;

	BigInt prefix_ = 0;

public:
	typedef Int ReturnType;

	FindRBeginWalker(Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		node->process(0, *this);

		return node->children_count() - 1;
	}

	template <Int Idx, typename Tree>
	void stream(const Tree* tree)
	{
		prefix_ += tree->sumWithoutLastElement();
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
