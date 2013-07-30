
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTORMAP_CONTAINER_WALKERS_HPP
#define _MEMORIA_CONTAINERS_VECTORMAP_CONTAINER_WALKERS_HPP

#include <memoria/prototypes/balanced_tree/bt_tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/balanced_tree/bt_walkers.hpp>

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
		iter.idx() 	= idx;

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

		Int entry_idx = 0;
		if (stream_ == 0)
		{
			entry_idx = idx;
		}
		else {
			// FIXME: correct entry_idx for data stream
			entry_idx = iter.cache().entry_idx();
		}

		iter.cache().setup(id_prefix, id_entry, size, base, entry_idx);

		return sum_;
	}


	void empty(Iterator& iter)
	{
		iter.idx()	= 0;
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
//	Accumulator local_prefix_;

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
		std::get<StreamIdx>(prefix_)[1 - index] 	+= tree->sum(1 - index, start, result.idx());

//		std::get<StreamIdx>(prefix_) += std::get<StreamIdx>(local_prefix_);
	}

//	struct ArrayStreamPrefixFn
//	{
//		Accumulator& prefix_;
//
//		ArrayStreamPrefixFn(Accumulator& accum): prefix_(accum) {}
//
//		template <Int StreamIdx, typename StreamTypes>
//		void stream(const PackedFSETree<StreamTypes>* tree, Int start, Int end)
//		{
//			std::get<StreamIdx>(prefix_)[0] += tree->sum(0, start, end);
//		}
//
//		template <Int StreamIdx, typename StreamTypes>
//		void stream(const PackedFSEArray<StreamTypes>* tree, Int start, Int end)
//		{}
//	};
//
//
//	template <typename Node>
//	void postProcessNode(const Node* node, Int start, Int end)
//	{
//		node->template processStream<1>(ArrayStreamPrefixFn(prefix_), start, end);
//	}


	void prepare(Iterator& iter)
	{
//		std::get<0>(prefix_)[0] = iter.cache().id_prefix();
//		std::get<0>(prefix_)[1] = iter.cache().blob_base();
	}

	BigInt finish(Iterator& iter, Int idx)
	{
		iter.idx() 	= idx;

		BigInt id_prefix 	= std::get<0>(prefix_)[0];
		BigInt base 		= std::get<0>(prefix_)[1];

		BigInt id_entry 	= 0;
		BigInt size 		= 0;

		BigInt global_pos	= base;

		Int entries			= iter.leaf_size(0);

		if (idx >=0 && idx < entries)
		{
			auto entry	= iter.entry();

			id_entry 	= entry.first;
			size		= entry.second;
		}

		iter.cache().setup(id_prefix, id_entry, base, size, idx, entries, global_pos);

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
class FindVMapEndWalkerBase: public FindRangeWalkerBase<Types> {
protected:
	typedef FindRangeWalkerBase<Types> 											Base;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;

	typedef typename Types::Accumulator 										Accumulator;


	Accumulator 	prefix_;
	Accumulator 	local_prefix_;

	Int stream_;

	IteratorMode mode_;

public:
	typedef Int ReturnType;
	typedef Int ResultType;

	FindVMapEndWalkerBase(Int stream, Container&, IteratorMode mode):
		stream_(stream),
		mode_(mode)
	{}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		return node->template processStreamRtn<0>(*this, node->level(), start);
	}

	template <Int StreamIdx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int level, Int start)
	{
		typedef PackedFSETree<TreeTypes> Tree;

		Int size = tree->content_size_from_start(0);
		Int idx = size - (mode_ == IteratorMode::FORWARD ? (level > 0) : 1);

		for (Int block = 0; block < Tree::Blocks; block++)
		{
			std::get<StreamIdx>(local_prefix_)[block] = tree->sum(block, idx);
		}

		std::get<StreamIdx>(prefix_) += std::get<StreamIdx>(local_prefix_);

		return idx;
	}


};


template <typename Types>
class FindVMapEndWalker: public FindVMapEndWalkerBase<Types> {
	typedef FindVMapEndWalkerBase<Types> 										Base;
	typedef typename Base::Container 											Container;
	typedef typename Base::Iterator 											Iterator;
public:
	FindVMapEndWalker(Int stream, Container& ctr):
		Base(stream, ctr, IteratorMode::FORWARD)
	{}

	void finish(Iterator& iter, Int idx)
	{
		iter.idx() = idx;

		iter.found() = false;

		BigInt id_prefix = std::get<0>(Base::prefix_)[0];
		BigInt base		 = std::get<0>(Base::prefix_)[1];

		BigInt global_pos 	= base;

		Int entries 	= iter.leaf_size(0);

		iter.cache().setup(id_prefix, 0, base, 0, idx, entries, global_pos);
	}

};


template <typename Types>
class FindVMapRBeginWalker: public FindVMapEndWalkerBase<Types> {
	typedef FindVMapEndWalkerBase<Types> 										Base;
	typedef typename Base::Container 											Container;
	typedef typename Base::Iterator 											Iterator;
public:
	FindVMapRBeginWalker(Int stream, Container& ctr):
		Base(stream, ctr, IteratorMode::BACKWARD)
	{}

	void finish(Iterator& iter, Int idx)
	{
		iter.idx() = idx;

		iter.found() = false;

		BigInt id_prefix = std::get<0>(Base::prefix_)[0];
		BigInt base		 = std::get<0>(Base::prefix_)[1];

		BigInt global_pos	= base;

		Int entries 	= iter.leaf_size(0);

		if(idx < entries)
		{
			auto entry		= iter.entry();

			auto id_entry 	= entry.first;
			auto size		= entry.second;

			iter.cache().setup(id_prefix, id_entry, base, size, idx, entries, global_pos);
		}
		else {
			iter.cache().setup(id_prefix, 0, base, 0, idx, entries, global_pos);
		}
	}
};





template <typename Types>
class FindVMapStartWalkerBase: public FindRangeWalkerBase<Types> {
protected:
	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

	typedef typename Types::Accumulator 	Accumulator;


public:
	typedef Int ReturnType;

	FindVMapStartWalkerBase(Int stream, Container&)
	{}

	template <typename Node>
	ReturnType treeNode(const Node* node, Int start)
	{
		return 0;
	}
};


template <typename Types>
class FindVMapBeginWalker: public FindVMapStartWalkerBase<Types> {
	typedef FindVMapStartWalkerBase<Types> 										Base;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;
public:

	FindVMapBeginWalker(Int stream, Container& ctr): Base(stream, ctr)
	{}

	void finish(Iterator& iter, Int idx)
	{
		iter.idx() = 0;

		if (!iter.isEnd())
		{
			auto entry = iter.entry();
			iter.cache().set(entry.first, entry.second, 0, iter.leaf_size(0), 0);
		}
	}
};

template <typename Types>
class FindVMapREndWalker: public FindVMapStartWalkerBase<Types> {
	typedef FindVMapStartWalkerBase<Types> 										Base;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;
public:

	FindVMapREndWalker(Int stream, Container& ctr): Base(stream, ctr)
	{}

	void finish(Iterator& iter, Int idx)
	{
		iter.idx() = -1;
	}
};


template <typename Types>
class PrevLeafWalker: public PrevLeafWalkerBase<Types, PrevLeafWalker<Types>> {

protected:

	typedef PrevLeafWalkerBase<Types, PrevLeafWalker<Types>> 					Base;
	typedef typename Base::Key 													Key;
	typedef typename Base::Position 											Position;
	typedef typename Base::Iterator												Iterator;

public:
	PrevLeafWalker(Int stream, Int index): Base(stream, index)
	{}
};



}
}

#endif
