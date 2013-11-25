
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_DBLMAP2_INNERMAP_C_API_HPP
#define MEMORIA_CONTAINERS_DBLMAP2_INNERMAP_C_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <functional>

namespace memoria {


MEMORIA_CONTAINER_PART_BEGIN(memoria::dblmap::InnerCtrApiName)

	typedef typename Base::Types                                                Types;


    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Key                                                 Key;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Element												Element;

    static const Int Streams                                                    = Types::Streams;

    BigInt size() const {
    	return self().sizes()[0];
    }

    Iterator seek(Key pos)
    {
    	return self().findGT(0, pos, 0);
    }

    BigInt remove(Iterator& from, Iterator& to)
    {
    	Accumulator keys;
    	self().removeEntries(from, to, keys);

    	return std::get<0>(keys)[1];
    }

    BigInt remove(Iterator& from)
    {
    	Accumulator keys;
    	self().removeMapEntry(from, keys);

    	return std::get<0>(keys)[1];
    }

//    bool insertMapEntry(Iterator& iter, const Element& element)
//    {
//    	auto& self      = this->self();
//    	NodeBaseG& leaf = iter.leaf();
//    	Int& idx        = iter.idx();
//
//    	bool refresh_prefix = false;
//
//    	if (!self.insertIntoLeaf(leaf, idx, element))
//    	{
//    		refresh_prefix = iter.split();
//    		if (!self.insertIntoLeaf(leaf, idx, element))
//    		{
//    			throw Exception(MA_SRC, "Second insertion attempt failed");
//    		}
//    	}
//
//    	self.updateParent(leaf, element.first);
//
//    	self.addTotalKeyCount(Position::create(0, 1));
//
//    	return refresh_prefix;
//    }

MEMORIA_CONTAINER_PART_END

}


#endif
