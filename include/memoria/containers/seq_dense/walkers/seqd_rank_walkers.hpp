
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_CONTAINERS_SEQDENSE_RANK_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_RANK_WALKERS_HPP

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

namespace memoria 	{
namespace seq_dense	{


template <typename Types>
class RankFWWalker: public bt::FindForwardWalkerBase<Types, RankFWWalker<Types>> {
	typedef bt::FindForwardWalkerBase<Types, RankFWWalker<Types>> 	Base;
	typedef typename Base::Key 													Key;

	BigInt rank_ = 0;

	Int symbol_;

public:
	typedef typename Base::Iterator												Iterator;
	typedef typename Base::ResultType											ResultType;

	RankFWWalker(Int stream, Int index, Key target): Base(stream, 0, target)
	{
		Base::search_type_ 	= SearchType::LT;
		symbol_ 			= index;
	}

	BigInt rank() const {
		return rank_;
	}

	template <Int Idx, typename Tree>
	ResultType stream(const Tree* tree, Int start) {
		return Base::template stream<Idx>(tree, start);
	}

	template <Int StreamIdx, typename StreamType, typename Result>
	void postProcessStream(const StreamType* stream, Int start, const Result& result)
	{
		Int size = stream->size();

		if (result.idx() < size)
		{
			rank_ += stream->sum(symbol_ + 1, start, result.idx());
		}
		else {
			rank_ += stream->sum(symbol_ + 1, start, size);
		}
	}

	template <Int Idx, typename StreamTypes>
	ResultType stream(const PkdFSSeq<StreamTypes>* seq, Int start)
	{
		MEMORIA_ASSERT_TRUE(seq != nullptr);

		auto& sum 		= Base::sum_;

		BigInt offset 	= Base::target_ - sum;


		Int	size 		= seq->size();

		if (start + offset < size)
		{
			rank_ += seq->rank(start, start + offset, symbol_);

			sum += offset;

			return start + offset;
		}
		else {
			rank_ += seq->rank(start, seq->size(), symbol_);

			sum += (size - start);

			return size;
		}
	}

	BigInt finish(Iterator& iter, Int idx)
	{
		iter.idx() = idx;

		return rank_;
	}
};




template <typename Types>
class RankBWWalker: public bt::FindBackwardWalkerBase<Types, RankBWWalker<Types>> {
	typedef bt::FindBackwardWalkerBase<Types, RankBWWalker<Types>> 					Base;
	typedef typename Base::Key 													Key;

	BigInt rank_ = 0;

	Int symbol_;

public:
	typedef typename Base::Iterator												Iterator;
	typedef typename Base::ResultType											ResultType;

	RankBWWalker(Int stream, Int index, Key target): Base(stream, 0, target)
	{
		Base::search_type_ 	= SearchType::LT;
		symbol_ 			= index;
	}

	BigInt rank() const {
		return rank_;
	}

	template <Int Idx, typename Tree>
	ResultType stream(const Tree* tree, Int start)
	{
		return Base::template stream<Idx>(tree, start);
	}

	template <Int StreamIdx, typename StreamType, typename Result>
	void postProcessStream(const StreamType* stream, Int start, const Result& result)
	{
		if (result.idx() >= 0)
		{
			rank_ += stream->sum(symbol_ + 1, result.idx() + 1, start + 1);
		}
		else {
			rank_ += stream->sum(symbol_ + 1, 0, start + 1);
		}
	}

	template <Int Idx, typename StreamTypes>
	ResultType stream(const PkdFSSeq<StreamTypes>* seq, Int start)
	{
		MEMORIA_ASSERT_TRUE(seq != nullptr);

		auto& sum 		= Base::sum_;
		BigInt offset 	= Base::target_ - sum;

		if (start - offset >= 0)
		{
			rank_ += seq->rank(start - offset, start, symbol_);

			sum += offset;
			return start - offset;
		}
		else {
			rank_ += seq->rank(0, start, symbol_);

			sum += start;
			return -1;
		}
	}

	BigInt finish(Iterator& iter, Int idx)
	{
		iter.idx() = idx;

		return rank_;
	}
};





}
}

#endif
