
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_IDX_MAP_CONTAINER_API_HPP
#define	_MEMORIA_MODELS_IDX_MAP_CONTAINER_API_HPP


#include <memoria/prototypes/btree/pages/tools.hpp>
#include <memoria/containers/map/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::idx_map::CtrApiName)

    typedef typename Base::Iterator                                             Iterator;
	typedef typename Base::Key                                             		Key;
	typedef typename Base::Value                                             	Value;
	typedef typename Base::Element                                             	Element;
	typedef typename Base::Accumulator                                          Accumulator;


    Iterator Find(Key key)
    {
    	Iterator iter = me()->FindLE(key, 0, true);

    	if (!iter.IsEnd())
    	{
    		if (key == iter)
    		{
    			return iter;
    		}
    		else {
    			return me()->End();
    		}
    	}
    	else {
    		return iter;
    	}
    }

    Iterator operator[](Key key)
    {
    	Iterator iter = me()->FindLE(key, 0, true);

    	if (key != iter)
    	{
    		Accumulator keys;
    		keys[0] = key;
    		me()->InsertEntry(iter, keys);
    	}

    	return iter;
    }

    bool Remove(Key key)
    {
    	Iterator iter = me()->FindLE(key, 0, true);
    	if (key == iter)
    	{
    		iter.Remove();
    		return true;
    	}
    	else {
    		return false;
    	}
    }

    void Insert(Iterator& iter, const Element& element)
    {
    	Accumulator delta = element.first - iter.prefix();

    	Element e(delta, element.second);

    	if (Base::Insert(iter, e))
    	{
    		iter.UpdateUp(-delta);
    	}
    }

MEMORIA_CONTAINER_PART_END

}


#endif
