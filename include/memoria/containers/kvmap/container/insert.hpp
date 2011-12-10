
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_KVMAP_MODEL_INSERT_HPP
#define	_MEMORIA_MODELS_KVMAP_MODEL_INSERT_HPP


#include <memoria/containers/kvmap/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::kvmap::InsertName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Page::ID                                                   ID;
    typedef typename Allocator::Transaction                                     Transaction;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::Counters                                            Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Types::Pages::NodeFactory                                  NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Types::Value                                                Value;

    static const Int Indexes                                                    = Types::Indexes;

    void SetValueForKey(Key key, const Value& value);
    bool RemoveByKey(Key key);


MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::models::kvmap::InsertName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::SetValueForKey(Key key, const Value& value)
{
	Iterator i = me()->FindLE(key, 0, true);

	if (i.IsFound())
	{
		if (i.GetKey(0) == key)
		{
			me()->SetLeafData(i.page(), i.key_idx(), value);
		}
		else {
			me()->InsertEntry(i, key, value);
		}
	}
	else {
		me()->InsertEntry(i, key, value);
	}
}

M_PARAMS
bool M_TYPE::RemoveByKey(Key key)
{
	Iterator i = me()->FindLE(key, 0, false);
	if (i.IsFound())
	{
		if (i.GetKey(0) == key)
		{
			me()->RemoveEntry(i);
			return true;
		}
	}
	return false;
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
