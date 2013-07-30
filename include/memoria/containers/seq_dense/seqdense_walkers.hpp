
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_WALKERS_HPP

#include <memoria/prototypes/balanced_tree/bt_walkers.hpp>

#include <memoria/core/packed2/packed_fse_searchable_seq.hpp>

namespace memoria 	{
namespace seq_dense	{

template <typename Types>
class SkipForwardWalker: public FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {
	typedef FindForwardWalkerBase<Types, SkipForwardWalker<Types>> 				Base;
	typedef typename Base::Key 													Key;

public:
	typedef typename Base::ResultType											ResultType;


	SkipForwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start) {
		return Base::template stream<Idx>(tree, start);
	}

	template <Int Idx, typename StreamTypes>
	ResultType stream(const PackedFSESearchableSeq<StreamTypes>* seq, Int start)
	{
		auto& sum = Base::sum_;

		BigInt offset = Base::target_ - sum;

		Int	size = seq != nullptr? seq->size() : 0;

		if (start + offset < size)
		{
			sum += offset;

			return start + offset;
		}
		else {
			sum += (size - start);

			return size;
		}
	}
};

template <typename Types>
class SkipBackwardWalker: public FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {
	typedef FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>>			Base;
	typedef typename Base::Key 													Key;

public:
	typedef typename Base::ResultType											ResultType;

	SkipBackwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{
		Base::search_type_ = SearchType::LT;
	}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start) {
		return Base::stream(tree, start);
	}


	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSESearchableSeq<TreeTypes>* seq, Int start)
	{
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
};




template <typename Types>
class SelectForwardWalker: public FindForwardWalkerBase<Types, SelectForwardWalker<Types>> {
	typedef FindForwardWalkerBase<Types, SelectForwardWalker<Types>> 			Base;
	typedef typename Base::Key 													Key;

	BigInt pos_ = 0;

public:
	typedef typename Base::ResultType											ResultType;


	SelectForwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{
		Base::search_type_ = SearchType::LE;
	}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start) {
		return Base::stream(tree, start);
	}

	template <Int StreamIdx, typename StreamType, typename Result>
	void postProcessStream(const StreamType* stream, Int start, const Result& result)
	{
		if (result.is_found())
		{
			pos_ += stream->sum(0, start, result.idx());
		}
		else {
			pos_ += stream->sum(0, start, stream->size());
		}
	}

	template <Int Idx, typename StreamTypes>
	ResultType stream(const PackedFSESearchableSeq<StreamTypes>* seq, Int start)
	{
		MEMORIA_ASSERT_TRUE(seq != nullptr);

		auto& sum 		= Base::sum_;
		auto symbol		= Base::index_ - 1;

		BigInt target 	= Base::target_ - sum;
		auto result 	= seq->selectFw(start, symbol, target);
		Int offset 		= result.is_found() ? result.idx() : seq->size();

		Int	size 		= seq->size();

		if (start + offset < size)
		{
			pos_ += offset;

			return start + offset;
		}
		else {
			pos_ += (size - start);

			return size;
		}
	}
};



template <typename Types>
class SelectBackwardWalker: public FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>> {
	typedef FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>>			Base;
	typedef typename Base::Key 													Key;


	BigInt pos_ = 0;

public:
	typedef typename Base::ResultType											ResultType;

	SelectBackwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{
		Base::search_type_ = SearchType::LE;
	}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start) {
		return Base::stream(tree, start);
	}

	template <Int StreamIdx, typename StreamType, typename Result>
	void postProcessStream(const StreamType* stream, Int start, const Result& result)
	{
		if (result.is_found())
		{
			pos_ += stream->sum(0, result.idx() + 1, start + 1);
		}
		else {
			pos_ += stream->sum(0, 0, start + 1);
		}
	}


	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSESearchableSeq<TreeTypes>* seq, Int start)
	{
		MEMORIA_ASSERT_TRUE(seq != nullptr);

		BigInt target 	= Base::target_ - Base::sum_;

		auto& sum 		= Base::sum_;
		auto symbol		= Base::index_ - 1;

		auto result 	= seq->selectBw(start, symbol, target);
		Int offset  	= result.is_found() ? result.idx() : -1;

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
};



template <typename Types>
class RankWalker: public FindForwardWalkerBase<Types, RankWalker<Types>> {
	typedef FindForwardWalkerBase<Types, RankWalker<Types>> 					Base;
	typedef typename Base::Key 													Key;

	BigInt rank_ = 0;

public:
	typedef typename Base::ResultType											ResultType;

	RankWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{
		Base::search_type_ = SearchType::LT;
	}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PackedFSETree<TreeTypes>* tree, Int start) {
		return Base::stream(tree, start);
	}

	template <Int StreamIdx, typename StreamType, typename Result>
	void postProcessStream(const StreamType* stream, Int start, const Result& result)
	{
		Int index = Base::index_;

		if (result.is_found())
		{
			rank_ += stream->sum(index, start, result.idx());
		}
		else {
			rank_ += stream->sum(index, start, stream->size());
		}
	}

	template <Int Idx, typename StreamTypes>
	ResultType stream(const PackedFSESearchableSeq<StreamTypes>* seq, Int start)
	{
		MEMORIA_ASSERT_TRUE(seq != nullptr);

		auto& sum 		= Base::sum_;
		auto symbol		= Base::index_ - 1;

		BigInt offset 	= Base::target_ - sum;


		Int	size 		= seq->size();

		if (start + offset < size)
		{
			rank_ += seq->rank(start, start + offset, symbol);

			sum += offset;

			return start + offset;
		}
		else {
			rank_ += seq->rank(start, seq->size(), symbol);

			sum += (size - start);

			return size;
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
