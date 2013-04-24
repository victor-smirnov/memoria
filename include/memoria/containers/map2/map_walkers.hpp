
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
	Int stream_;

	WalkDirection direction_;



public:
	FindWalkerBase(Int stream, Int key_num, Key key):
		key_(key), key_num_(key_num), stream_(stream)
	{}

	const WalkDirection& direction() const {
		return direction_;
	}

	WalkDirection& direction() {
		return direction_;
	}


	void finish(Iterator& iter, Int idx)
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

	FindLTWalker(Int stream, Int key_num, Int key): Base(stream, key_num, key)
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
	FindLEWalker(Int stream, Int key_num, Key key): Base(stream, key_num, key)
	{}

	typedef Int ResultType;
	typedef Int ReturnType;

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		return node->find(Base::stream_, *this, node->level(), start);
	}



	template <Int Idx, typename Tree>
	Int stream(const Tree* tree, Int level, Int start)
	{
		auto& key		= Base::key_;
		auto& prefix 	= Base::prefix_;

		auto target 	= key - prefix;

		auto result 	= tree->findLEForward(target);

		prefix += result.prefix();

		return result.idx();
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

	FindEndWalker(Int stream, Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		node->process(0, *this, node->level(), start);

		return node->size(0) - 1;
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


	void finish(Iterator& iter, Int idx)
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

	FindREndWalker(Int stream, Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		return 0;
	}

	void finish(Iterator& iter, Int idx)
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

	FindBeginWalker(Int stream, Container&) {}


	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		return 0;
	}

	void finish(Iterator& iter, Int idx)
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

	FindRBeginWalker(Int stream, Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		node->process(0, *this);

		return node->size(0) - 1;
	}

	template <Int Idx, typename Tree>
	void stream(const Tree* tree)
	{
		prefix_ += tree->sumWithoutLastElement();
	}


	void finish(Iterator& iter, Int idx)
	{
		iter.key_idx() = idx;

		iter.cache().setup(prefix_);
	}
};


}
}

#endif
