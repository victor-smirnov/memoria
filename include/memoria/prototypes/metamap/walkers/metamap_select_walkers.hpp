
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_METAMAP_SELECT_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_SELECT_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_select_walkers.hpp>

namespace memoria {
namespace metamap {



template <
	typename Types,
	typename IteratorPrefixFn = bt1::DefaultIteratorPrefixFn
>
class SelectForwardWalker: public bt1::SelectForwardWalkerBase<
									Types,
									IteratorPrefixFn,
									SelectForwardWalker<Types, IteratorPrefixFn>> {

	using Base 	= bt1::SelectForwardWalkerBase<Types, IteratorPrefixFn, SelectForwardWalker<Types, IteratorPrefixFn>>;
	using Key 	= typename Base::Key;

	bool hidden_;

public:
	using ResultType = Int;

	SelectForwardWalker(Int stream, Int branch_index, Int symbol, bool hidden, Key target):
		Base(stream, branch_index, symbol, target),
		hidden_(hidden)
	{}

	template <Int StreamIdx, typename Seq>
	SelectResult select(const Seq* seq, Int start, Int symbol, BigInt rank)
	{
		if (hidden_)
		{
			return seq->h_selectFw(start, symbol, rank);
		}
		else {
			return seq->selectFw(start, symbol, rank);
		}
	}
};









template <
	typename Types,
	typename IteratorPrefixFn = bt1::DefaultIteratorPrefixFn
>
class SelectBackwardWalker: public bt1::SelectBackwardWalkerBase<
									Types,
									IteratorPrefixFn,
									SelectBackwardWalker<Types, IteratorPrefixFn>> {

	using Base 	= bt1::SelectBackwardWalkerBase<Types, IteratorPrefixFn, SelectBackwardWalker<Types, IteratorPrefixFn>>;
	using Key 	= typename Base::Key;

	bool hidden_;

public:

	SelectBackwardWalker(Int stream, Int branch_index, Int symbol, bool hidden, Key target):
		Base(stream, branch_index, symbol, target),
		hidden_(hidden)
	{}

	template <Int StreamIdx, typename Seq>
	SelectResult select(const Seq* seq, Int start, Int symbol, BigInt rank)
	{
		if (hidden_)
		{
			return seq->h_selectBw(start, symbol, rank);
		}
		else {
			return seq->selectBw(start, symbol, rank);
		}
	}
};


}
}

#endif
