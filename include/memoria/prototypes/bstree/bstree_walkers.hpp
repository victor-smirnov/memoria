
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BSTREE_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BSTREE_WALKERS_HPP

#include <memoria/prototypes/btree/tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <ostream>

namespace memoria       {
namespace bstree        {


template <typename Types>
class FindWalkerBase {
protected:
	typedef typename Types::Key 												Key;
	typedef Iter<typename Types::IterTypes> 									Iterator;

	Key key_;
	Int key_num_;

	Key prefix_;
	Int idx_;

	WalkDirection direction_;

	Int start_;

public:
	FindWalkerBase(Key key, Int key_num):
		key_(key), key_num_(key_num), prefix_(0), idx_(0)
	{}

	const WalkDirection& direction() const {
		return direction_;
	}

	WalkDirection& direction() {
		return direction_;
	}

	const Int& start() const {
		return start_;
	}

	Int& start() {
		return start_;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;
		iter.init();
	}

	void empty(Iterator& iter)
	{
		// do nothing
	}

	Int idx() const {
		return idx_;
	}
};


}
}

#endif

