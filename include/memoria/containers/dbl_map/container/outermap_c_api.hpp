
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_DBLMAP2_OUTERMAP_C_API_HPP
#define MEMORIA_CONTAINERS_DBLMAP2_OUTERMAP_C_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <functional>
#include <tuple>

namespace memoria {


MEMORIA_CONTAINER_PART_BEGIN(memoria::dblmap::OuterCtrApiName)
    typedef typename Base::Types                                                Types;


    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Key                                                  Key;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename std::tuple_element<0, Accumulator>::type					Entry0;

    static const Int Streams                                                    = Types::Streams;

    BigInt size() const {
    	return self().sizes()[0];
    }

    Iterator find(Key key)
    {
    	Iterator iter = self().findGE(0, key, 0);

    	if (!iter.isEnd())
    	{
    		if (iter.is_found_eq(key))
    		{
    			return iter;
    		}
    		else {
    			return self().End();
    		}
    	}
    	else {
    		return iter;
    	}
    }

    Iterator findKeyGE(Key key)
    {
    	return self().findGE(0, key, 0);
    }

    Iterator findKeyLE(Key key)
    {
    	Iterator iter = self().findGE(0, key, 0);

    	if (iter.isEnd() || iter.key() > key)
    	{
    		iter--;

    		if (iter.isBegin())
    		{
    			iter.idx() = 0;
    		}
    	}

    	return iter;
    }

    Iterator findKeyLT(Key key)
    {
    	Iterator iter = self().findGE(0, key, 0);

    	if (iter.isEnd() || iter.key() >= key)
    	{
    		iter--;

    		if (iter.isBegin())
    		{
    			iter.idx() = 0;
    		}
    	}

    	return iter;
    }




    bool remove(Key key)
    {
    	Iterator iter = self().findGE(0, key, 0);

    	if (iter.is_found_eq(key))
    	{
    		iter.remove();
    		return true;
    	}
    	else {
    		return false;
    	}
    }

    void remove(Iterator& from, Iterator& to)
    {
    	Accumulator keys;
    	self().removeEntries(from, to, keys);

    	if (!to.isEnd())
    	{
    		std::get<0>(keys)[1] = 0;
    		to.updateUp(keys);
    	}
    }

    void remove(Iterator& from)
    {
    	Accumulator keys;
    	self().removeMapEntry(from, keys);

    	if (!from.isEnd())
    	{
    		std::get<0>(keys)[1] = 0;
    		from.updateUp(keys);
    	}
    }

    void insert(Iterator& iter, const Entry0& element)
    {
    	Accumulator accum;

    	std::get<0>(accum) = element;
    	std::get<0>(accum)[0] -= std::get<0>(iter.prefixes())[0];

    	if (self().insertMapEntry(iter, accum))
    	{
    		std::get<0>(accum)[1] = 0;
    		iter.updateUp(-accum);
    	}
    }

    void insert(const Entry0& element)
    {
    	auto iter = self().findKeyGE(element[0]);
    	self().insert(iter, element);
    }


MEMORIA_CONTAINER_PART_END

}


#endif
