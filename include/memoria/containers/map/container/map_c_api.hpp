
// Copyright Victor Smirnov 2011 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAP_CTR_API_HPP
#define _MEMORIA_CONTAINERS_MAP_CTR_API_HPP


#include <memoria/containers/map/map_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::map::CtrApiName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::TreeNodePage                                         TreeNodePage;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::NodeDispatcher                                       NodeDispatcher;
	typedef typename Base::RootDispatcher                                       RootDispatcher;
	typedef typename Base::LeafDispatcher                                       LeafDispatcher;
	typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;
	typedef typename Base::Element                                              Element;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Base::TreePath                                             TreePath;
	typedef typename Base::TreePathItem                                         TreePathItem;

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;

	static const Int MAIN_STREAM												= Types::MAIN_STREAM;



	BigInt size() const {
		return self().sizes()[0];
	}

    Iterator find(Key key)
    {
        Iterator iter = self().findLE(MAIN_STREAM, key, 0);

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


    Iterator operator[](Key key)
    {
        Iterator iter = self().findLE(MAIN_STREAM, key, 0);

        if (iter.isEnd() || key != iter.key())
        {
        	Accumulator keys;
            std::get<0>(keys)[0] = key;
            self().insert(iter, keys);

            iter.prev();
        }

        return iter;
    }

    bool remove(Key key)
    {
    	Iterator iter = self().findLE(MAIN_STREAM, key, 0);

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

    void insert(Iterator& iter, const Element& element)
    {
        Accumulator delta = element.first - iter.prefixes();

        Element e(delta, element.second);

        if (Base::insert(iter, e))
        {
            iter.updateUp(-delta);
        }
    }

    void insertRaw(Iterator& iter, const Element& element)
    {
        Base::insert(iter, element);
    }

    bool contains(Key key)
    {
        return !self().find(key).isEnd();
    }

    bool contains1(Key key)
    {
        return !self().find1(key).isEnd();
    }

    bool removeEntry1(Iterator& iter, Accumulator& keys)
    {
        bool result = Base::removeEntry(iter, keys);

        if (!iter.isEnd())
        {
            iter.updateUp(keys);
        }

        return result;
    }

MEMORIA_CONTAINER_PART_END

}


#endif
