
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
class MapFindWalker: public FindForwardWalkerBase<Types, MapFindWalker<Types>> {

	typedef FindForwardWalkerBase<Types, MapFindWalker<Types>> 					Base;
	typedef typename Base::Key 													Key;
	typedef typename Types::Accumulator 										Accumulator;
	typedef Iter<typename Types::IterTypes> 									Iterator;

	Accumulator prefix_;

public:
	MapFindWalker(Key key):
		Base(0, 0, key)
	{
		Base::search_type() = SearchType::LE;
	}


	template <Int StreamIdx, typename StreamTypes, typename SearchResult>
	void postProcessStream(const PackedFSETree<StreamTypes>* tree, Int start, const SearchResult& result)
	{
		auto& index 	= Base::index_;

		std::get<StreamIdx>(prefix_)[index] 		+= result.prefix();
		std::get<StreamIdx>(prefix_)[1 - index] 	+= tree->sum(index, start, result.idx());
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


		if (idx >=0 && idx < iter.leafSize(0))
		{
			auto entry	= iter.entry();

			id_entry 	= entry.first;
			size		= entry.second;
		}

		iter.cache().setup(id_prefix, id_entry, size, base);

		return Base::sum_;
	}
};


template <typename Types>
class SkipForwardWalker: public FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {

	typedef FindForwardWalkerBase<Types, SkipForwardWalker<Types>> 				Base;
	typedef typename Types::Key 												Key;
	typedef typename Types::Accumulator 										Accumulator;

public:
	SkipForwardWalker(Int stream, Int index, Key distance):
		Base(stream, index, distance)
	{
		Base::search_type() = SearchType::LT;
	}
};


template <typename Types>
class SkipBackwardWalker: public FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {

	typedef FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> 			Base;
	typedef typename Types::Key 												Key;
	typedef typename Types::Accumulator 										Accumulator;

public:
	SkipBackwardWalker(Int stream, Int index, Key distance):
		Base(stream, index, distance)
	{
		Base::search_type() = SearchType::LT;
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

		for (Int block = 0; block < Tree::Blocks; block++)
		{
			std::get<StreamIdx>(local_prefix_)[block] = tree->sum(block);
		}

		std::get<StreamIdx>(prefix_) += std::get<StreamIdx>(local_prefix_);

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
