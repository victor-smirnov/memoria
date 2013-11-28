
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SKIP_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SKIP_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>

namespace memoria {
namespace bt1 	  {

template <
	typename Types,
	typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class SkipForwardWalker: public FindForwardWalkerBase<Types, IteratorPrefixFn, SkipForwardWalker<Types, IteratorPrefixFn>> {

	using Base 	= FindForwardWalkerBase<Types, IteratorPrefixFn, SkipForwardWalker<Types, IteratorPrefixFn>>;
	using Key 	= typename Base::Key;

public:
	SkipForwardWalker(Int stream, Int block, Key target):
		Base(stream, block, target, SearchType::GT)
	{}
};




template <
	typename Types,
	typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class SkipBackwardWalker: public FindBackwardWalkerBase<
									Types,
									IteratorPrefixFn,
									SkipBackwardWalker<Types, IteratorPrefixFn>> {

	using Base 	= FindBackwardWalkerBase<Types, IteratorPrefixFn, SkipBackwardWalker<Types, IteratorPrefixFn>>;
	using Key 	= typename Base::Key;

public:
	SkipBackwardWalker(Int stream, Int block, Key target):
		Base(stream, block, target, SearchType::GT)
	{}
};


}
}

#endif
