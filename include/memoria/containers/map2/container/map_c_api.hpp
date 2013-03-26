
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_IDX_MAP2_CONTAINER_API_HPP
#define _MEMORIA_MODELS_IDX_MAP2_CONTAINER_API_HPP


#include <memoria/containers/map2/names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::map2::CtrApiName)

    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;
    typedef typename Base::Accumulator                                          Accumulator;


    Iterator find(Key key)
    {
        Iterator iter = me()->findLE(key, 0);

        if (!iter.isEnd())
        {
            if (key == iter.key())
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
        Iterator iter = me()->findLE(key, 0);

        if (iter.isEnd() || key != iter.key())
        {
        	Accumulator keys;
            keys[0] = key;
            me()->insert(iter, keys);

            iter.prevKey();
        }

        return iter;
    }

    bool remove(Key key)
    {
        Iterator iter = me()->findLE(key, 0);

        if (key == iter.key(0))
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
        me()->removeEntries(from, to, keys);

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
        return !me()->find(key).isEnd();
    }

    bool contains1(Key key)
    {
        return !me()->find1(key).isEnd();
    }

    bool removeEntry(Iterator& iter, Accumulator& keys)
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
