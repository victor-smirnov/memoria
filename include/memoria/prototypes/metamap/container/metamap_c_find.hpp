
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_METAMAP_CTR_FIND_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_CTR_FIND_HPP


#include <memoria/prototypes/metamap/metamap_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::metamap::CtrFindName)

    typedef typename Base::Types                                                Types;


    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef ValuePair<Accumulator, Value>                                   	Element;

    typedef typename Types::CtrSizeT                                           	CtrSizeT;

    typedef typename Types::Entry                                         		MapEntry;

    static const Int Streams                                                    = Types::Streams;

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    Iterator find(Key key)
    {
        Iterator iter = self().findGE(0, key, 1);

        if (!iter.isEnd())
        {
            if (key == iter.key())
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
    	return self().findGE(0, key, 1);
    }

    Iterator findKeyLE(Key key)
    {
    	Iterator iter = self().findGE(0, key, 1);

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
    	Iterator iter = self().findGE(0, key, 1);

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



    Iterator operator[](Key key)
    {
        Iterator iter = self().findGE(0, key, 1);

        if (iter.isEnd() || key != iter.key())
        {
            MapEntry entry;
            entry.key() = key;

        	self().insertEntry(iter, entry);

            iter--;
        }

        return iter;
    }

    Iterator selectLabel(Int symbol, CtrSizeT rank)
    {
    	typename Types::template SelectForwardWalker<Types> walker(0, 2 + symbol, symbol, false, rank);

    	return self().find0(0, walker);
    }

    Iterator selectHiddenLabel(Int symbol, CtrSizeT rank)
    {
    	typename Types::template SelectForwardWalker<Types> walker(0, 2 + symbol, symbol, true, rank);

    	return self().find0(0, walker);
    }

MEMORIA_CONTAINER_PART_END

}


#endif
