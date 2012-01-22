
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_PROTOTYPES_BTREE_ITERATOR_MULTISKIP_H
#define MEMORIA_PROTOTYPES_BTREE_ITERATOR_MULTISKIP_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/vapi/models/logs.hpp>

#include <memoria/prototypes/btree/names.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/tools/walkers.hpp>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(IteratorMultiskipName)

	typedef typename Base::NodeBase                                             	NodeBase;
	typedef typename Base::Container                                                Container;
	typedef typename Container::Key                                                 Key;
	typedef typename Base::Container::Page                                          PageType;
	typedef typename Base::Container::ID                                            ID;
	typedef typename Container::Types::Pages::NodeDispatcher                        NodeDispatcher;


	BigInt SkipKeyFw(BigInt distance);
	BigInt SkipKeyBw(BigInt distance);

MEMORIA_ITERATOR_PART_END

#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::btree::IteratorMultiskipName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::SkipKeyFw(BigInt distance)
{
	typedef KeyCounterWalker<Container, true> Walker;
	return me()->template skip_keys_fw<Walker>(distance);
}

M_PARAMS
BigInt M_TYPE::SkipKeyBw(BigInt distance)
{
	typedef KeyCounterWalker<Container, false> Walker;
	return me()->template skip_keys_bw<Walker>(distance);
}


#undef M_TYPE
#undef M_PARAMS

}


#endif
