
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_CONTAINERS_SEQDENSE_SELECT_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_SELECT_WALKERS_HPP

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/core/packed/packed_fse_searchable_seq.hpp>

namespace memoria 	{
namespace seq_dense	{




template <typename Types>
class SelectForwardWalker: public bt::FindForwardWalkerBase<Types, SelectForwardWalker<Types>> {
	typedef bt::FindForwardWalkerBase<Types, SelectForwardWalker<Types>> 		Base;
	typedef typename Base::Key 																Key;

	BigInt pos_ = 0;

public:
	typedef typename Base::ResultType											ResultType;


	SelectForwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{
		Base::search_type_ = SearchType::LE;
	}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PkdFTree<TreeTypes>* tree, Int start) {
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
	ResultType stream(const PkdFSSeq<StreamTypes>* seq, Int start)
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
class SelectBackwardWalker: public bt::FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>> {
	typedef bt::FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>>		Base;
	typedef typename Base::Key 																Key;


	BigInt pos_ = 0;

public:
	typedef typename Base::ResultType											ResultType;

	SelectBackwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
	{
		Base::search_type_ = SearchType::LE;
	}

	template <Int Idx, typename TreeTypes>
	ResultType stream(const PkdFTree<TreeTypes>* tree, Int start) {
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
	ResultType stream(const PkdFSSeq<TreeTypes>* seq, Int start)
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

}
}

#endif
