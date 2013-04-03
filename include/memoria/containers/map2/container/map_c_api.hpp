
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_IDX_MAP2_CONTAINER_API_HPP
#define _MEMORIA_MODELS_IDX_MAP2_CONTAINER_API_HPP


#include <memoria/containers/map2/map_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::map2::CtrApiName)

	typedef typename Base::WrappedCtr::Types                                  	WTypes;
	typedef typename Base::WrappedCtr::Allocator                              	Allocator;

	typedef typename Base::WrappedCtr::ID                                     	ID;

	typedef typename WTypes::NodeBase                                           NodeBase;
	typedef typename WTypes::NodeBaseG                                          NodeBaseG;

	typedef typename Base::Iterator                                             Iterator;


	typedef typename WTypes::Pages::NodeDispatcher                              NodeDispatcher;
	typedef typename WTypes::Pages::RootDispatcher                              RootDispatcher;
	typedef typename WTypes::Pages::LeafDispatcher                              LeafDispatcher;
	typedef typename WTypes::Pages::NonLeafDispatcher                           NonLeafDispatcher;


	typedef typename WTypes::Key                                                Key;
	typedef typename WTypes::Value                                              Value;
	typedef typename WTypes::Element                                            Element;

	typedef typename WTypes::Metadata                                           Metadata;

	typedef typename WTypes::Accumulator                                        Accumulator;

	typedef typename WTypes::TreePath                                           TreePath;
	typedef typename WTypes::TreePathItem                                       TreePathItem;


	Iterator Begin() {
		return Iterator(self(), self().ctr().Begin());
	}

	Iterator begin() {
		return Iterator(self(), self().ctr().begin());
	}

	IterEndMark endm() const {
		return IterEndMark();
	}

	Iterator RBegin() {
		return Iterator(self(), self().ctr().RBegin());
	}

	Iterator End() {
		return Iterator(self(), self().ctr().End());
	}

	Iterator REnd() {
		return Iterator(self(), self().ctr().REnd());
	}

	BigInt getSize() const {
		return self().ctr().getSize();
	}

	BigInt size() const {
		return self().ctr().getSize();
	}

    Iterator find(Key key)
    {
        Iterator iter = self().findLE(key, 0);

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
        Iterator iter(self(), self().ctr().findLE(key, 0));

        if (iter.isEnd() || key != iter.key())
        {
        	Accumulator keys;
            keys[0] = key;
            self().insert(iter, keys);

            iter.prev();
        }

        return iter;
    }

    bool remove(Key key)
    {
    	Iterator iter(self(), self().ctr().findLE(key, 0));

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
        Accumulator delta = element.first - iter.iter().prefixes();

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
