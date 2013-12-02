
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
	using ResultType = Int;

	SkipForwardWalker(Int stream, Int block, Key target):
		Base(stream, block, block, target, SearchType::GT)
	{}


	template <Int StreamIdx, typename Array>
	ResultType find_leaf(const Array* array, Int start)
	{
		auto& sum = Base::sum_;

		BigInt offset = Base::target_ - sum;

		if (array != nullptr)
		{
			Int size = array->size();

			IteratorPrefixFn fn;

			if (start + offset < size)
			{
				sum += offset;

				fn.processLeafFw(array, std::get<StreamIdx>(Base::prefix_), start, start + offset, 0, offset);

				this->end_ = false;

				return start + offset;
			}
			else {
				sum += (size - start);

				fn.processLeafFw(array, std::get<StreamIdx>(Base::prefix_), start, size, 0, size - start);

				this->end_ = true;

				return size;
			}
		}
		else {
			return 0;
		}
	}

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
	using ResultType = Int;

	SkipBackwardWalker(Int stream, Int block, Key target):
		Base(stream, block, block, target, SearchType::GT)
	{}


	template <Int StreamIdx, typename Array>
	ResultType find_leaf(const Array* array, Int start)
	{
		BigInt offset = Base::target_ - Base::sum_;

		auto& sum = Base::sum_;

		if (array != nullptr)
		{
			IteratorPrefixFn fn;

			if (start - offset >= 0)
			{
				sum += offset;

				fn.processLeafBw(array, std::get<StreamIdx>(Base::prefix_), start - offset, start, 0, offset);

				this->end_ = false;

				return start - offset;
			}
			else {
				sum += start;

				fn.processLeafBw(array, std::get<StreamIdx>(Base::prefix_), 0, start, 0, start);

				this->end_ = true;

				return -1;
			}
		}
		else {
			return 0;
		}
	}
};


}
}

#endif
