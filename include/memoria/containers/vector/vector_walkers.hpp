
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTOR_CONTAINER_WALKERS_HPP
#define _MEMORIA_CONTAINERS_VECTOR_CONTAINER_WALKERS_HPP

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/balanced_tree/baltree_walkers.hpp>

#include <memoria/prototypes/balanced_tree/nodes/leaf_node.hpp>

#include <ostream>

namespace memoria       {
namespace mvector       {


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


	void finish(Iterator& iter, Int idx)
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

	FindEndWalker(Int stream, Container&) {}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
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
	ReturnType treeNode(const Node* node)
	{
		return 0;
	}

	void finish(Iterator& iter, Int idx)
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


	FindBeginWalker(Int stream, Container&) {}


	template <typename Node>
	ReturnType treeNode(const Node* node)
	{
		return 0;
	}

	void finish(Iterator& iter, Int idx)
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
	FindRBeginWalker(Int stream, Container&) {}

	typedef Int ReturnType;



	template <typename Node>
	ReturnType treeNode(const Node* node)
	{
		VectorAdd(prefix_, node->maxKeys() - node->keysAt(node->children_count() - 1));

		return node->children_count() - 1;
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
