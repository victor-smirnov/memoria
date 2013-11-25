
// Copyright Victor Smirnov 2011 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_SMRKMAP_CTR_API_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP_CTR_API_HPP


#include <memoria/containers/smrk_map/smrkmap_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::smrk_map::CtrApiName)

    typedef typename Base::Types                                                Types;


    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef ValuePair<Accumulator, Value>                                   	Element;

    static const Int Streams                                                    = Types::Streams;

    BigInt size() const {
        return self().sizes()[0];
    }

    Iterator find(Key key)
    {
        Iterator iter = self().findGE(0, key, 0);

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



    Iterator operator[](Key key)
    {
        Iterator iter = self().findGE(0, key, 0);

        if (iter.isEnd() || key != iter.key())
        {
            Accumulator keys;
            std::get<0>(keys)[1] = key;
            self().insert(iter, keys, 0);

            iter.prev();
        }

        return iter;
    }

    Iterator insertIFNotExists(Key key)
    {
    	Iterator iter = self().findGE(0, key, 0);

    	if (iter.isEnd() || key != iter.key())
    	{
    		Accumulator keys;
    		std::get<0>(keys)[0] = key;
    		self().insert(iter, keys);

    		iter.prev();
    	}
    	else {
    		throw Exception(MA_SRC, "Inserted Key already exists");
    	}

    	return iter;
    }

    bool remove(Key key)
    {
        Iterator iter = self().findGE(0, key, 0);

        if (key == iter.key())
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
            to.updateUp(keys);
        }

        to.cache().initState();
    }

    void insert(Iterator& iter, const Element& element, Int mark)
    {
        Accumulator delta = element.first - iter.prefixes();

        Element e(delta, element.second);

        if (self().insertMapEntry(iter, e, mark))
        {
            iter.updateUp(-delta);
        }
    }

    bool contains(Key key)
    {
        return !self().find(key).isEnd();
    }

    bool removeEntry1(Iterator& iter, Accumulator& keys)
    {
        bool result = self().removeMapEntry(iter, keys);

        if (!iter.isEnd())
        {
            iter.updateUp(keys);
        }

        return result;
    }

    Iterator select(Int mark, BigInt rank)
    {
    	auto& self = this->self();

    	MEMORIA_ASSERT(rank, >=, 1);
    	MEMORIA_ASSERT(mark, >=, 0);

    	typename Types::template SelectFwWalker<Types> walker(0, mark, rank);

    	auto iter = self.find0(0, walker);

    	return iter;
    }

    BigInt rank(Int mark)
    {
    	return self().End().rank(mark);
    }

MEMORIA_CONTAINER_PART_END

}


#endif
