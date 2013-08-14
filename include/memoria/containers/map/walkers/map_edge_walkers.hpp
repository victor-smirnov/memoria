
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAP_EDGE_WALKERS_HPP
#define _MEMORIA_CONTAINERS_MAP_EDGE_WALKERS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <ostream>

namespace memoria       {
namespace map        	{

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

	FindEndWalker(Int stream, const Container&) {}

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
			prefix_ += tree->sumWithoutLastElement(0);
		}
		else {
			prefix_ += tree->sum(0);
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

	FindREndWalker(Int stream, const Container&) {}

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

	FindBeginWalker(Int stream, const Container&) {}


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

	FindRBeginWalker(Int stream, const Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		node->process(0, *this);

		return node->size(0) - 1;
	}

	template <Int Idx, typename Tree>
	void stream(const Tree* tree)
	{
		prefix_ += tree->sumWithoutLastElement(0);
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
