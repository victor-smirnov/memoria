
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_PROTOTYPES_BSTREE_ITERATOR_MULTISKIP_H
#define MEMORIA_PROTOTYPES_BSTREE_ITERATOR_MULTISKIP_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/vapi/models/logs.hpp>

#include <memoria/prototypes/btree/names.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/tools/walkers.hpp>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::bstree::IteratorMultiskipName)

	typedef typename Base::NodeBase                                             	NodeBase;
	typedef typename Base::Container                                                Container;
	typedef typename Container::Key                                                 Key;
	typedef typename Base::Container::Page                                          PageType;
	typedef typename Base::Container::ID                                            ID;
	typedef typename Container::Types::Pages::NodeDispatcher                        NodeDispatcher;

	static const Int Indexes = Container::Indexes;

	BigInt SkipKeyFw(BigInt distance);
	BigInt SkipKeyBw(BigInt distance);

MEMORIA_ITERATOR_PART_END

#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::bstree::IteratorMultiskipName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::SkipKeyFw(BigInt distance)
{
	if (me()->page() == NULL || distance == 0)
	{
		return 0;
	}
	else {
		typedef KeyCounterWithSumWalker<Container, true> Walker;

		Walker walker(distance, me()->model());

		if (me()->WalkFw(me()->page(), me()->key_idx(), walker))
		{
			//TODO: check is EOF case is handled properly (prefixes);
			me()->key_idx()++;
		}

		for (Int c = 0; c < Indexes; c++)
		{
			me()->prefix(c) += walker.keys(c);
		}

		me()->ReHash();
		return walker.sum();
	}
}

M_PARAMS
BigInt M_TYPE::SkipKeyBw(BigInt distance)
{
	if (me()->page() == NULL)
	{
		return 0;
	}
	else {
		typedef KeyCounterWithSumWalker<Container, false> Walker;
		Walker walker(distance, me()->model());

		Key keys[Indexes];
		for (Int c = 0; c < Indexes; c++)
		{
			keys[c] = me()->GetRawKey(c);
		}

		if (me()->WalkBw(me()->page(), me()->key_idx(), walker))
		{
			//TODO: check is BOF case is handled properly (prefixes);
			me()->key_idx() = -1;
		}

		for (Int c = 0; c < Indexes; c++)
		{
			me()->prefix(c) -= walker.keys(c) + me()->GetRawKey(c) - keys[c];
		}


		me()->ReHash();
		return walker.sum();
	}
}


#undef M_TYPE
#undef M_PARAMS

}


#endif
