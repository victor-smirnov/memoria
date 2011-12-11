
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_MODEL_MODEL_API_HPP
#define _MEMORIA_MODELS_IDX_MAP_MODEL_MODEL_API_HPP

#include <memoria/containers/idx_map/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {


MEMORIA_CONTAINER_PART_BEGIN(memoria::models::idx_map::ContainerApiName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;


    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::Counters                                             Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Node2RootMap                                         Node2RootMap;
    typedef typename Base::Root2NodeMap                                         Root2NodeMap;

    typedef typename Base::NodeFactory                                          NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Base::ApiKeyType                                           ApiKeyType;
    typedef typename Base::ApiValueType                                         ApiValueType;

    static const Int Indexes                                                    = Base::Indexes;

    bool Find(BigInt index, Int c, Int search_type, Value& v);
    void Put(Key key, const Value& value = ID(0));
    void Insert(Key* keys, const Value& value, Int idx);

    bool Remove(Key index, Int key_num = 0)
    {
        Iterator i = me()->FindLE(index, key_num, false);
        return me()->RemoveEntry(i);
    }

    bool Remove(Key from_idx, Key to_idx, Int key_num = 0)
    {
        return me()->RemoveAllEntries(from_idx, to_idx, key_num, memoria::vapi::IdxMap::LE);
    }

private:
    bool find_lt(Key index, Int c, Value& value);
    bool find_le(BigInt index, Int c, Value& value);
   
MEMORIA_CONTAINER_PART_END





#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::models::idx_map::ContainerApiName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS


// -------------- API SECTION ------------------------


M_PARAMS
bool M_TYPE::Find(BigInt index, Int c, Int search_type, Value& v)
{
	if (search_type == memoria::vapi::IdxMap::LT) {
		return find_lt(index, c, v);
	}
	else if (search_type == memoria::vapi::IdxMap::LE) {
		return find_le(index, c, v);
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, "Invalid search_type value", search_type);
	}
}

M_PARAMS
void M_TYPE::Put(Key key, const Value& value)
{
	Key keys[Indexes];

	keys[0] = key;

	//FIXME:
	for (Int c = 1; c < Indexes; c++) {
		keys[c] = 0;
	}

	me()->Insert(keys, value, 0);
}


M_PARAMS
void M_TYPE::Insert(Key* keys, const Value& value, Int idx)
{
	Iterator i = me()->FindLE(keys[idx], idx, true);

	for (Int c = 0; c < Indexes; c++) keys[c] -= i.prefix(c);

	if (!i.IsEmpty()) {
		MEMORIA_TRACE(me(), "Insert into", i.page()->id(), "at", i.key_idx(), "key", keys[0], "prefix", i.prefix(0));
	}
	else {
		MEMORIA_TRACE(me(), "Insert into empty map", keys[0]);
	}

	if (i.IsEnd() || i.IsEmpty()) {
		me()->InsertEntry(i, keys, value);
	}
	else if (keys[0] == i.GetKey(0)) {
		me()->SetLeafData(i.page(), i.key_idx(), value);
	}
	else {
		me()->InsertEntry(i, keys, value);
	}
}






// -------------- PRIVATE SECTION ------------------------


M_PARAMS
bool M_TYPE::find_le(BigInt index, Int c, Value& value)
{
	Iterator i = me()->FindLE(index, c, false);

	if (!i.IsFound()) {
		return false;
	}
	else {
		value = i.GetData();
		return true;
	}
}

M_PARAMS
bool M_TYPE::find_lt(Key index, Int c, Value& value)
{
	Iterator i = me()->FindLT(index, c, false);

	if (!i.IsFound()) {
		return false;
	}
	else {
		value = i.GetData();
		return true;
	}
}



#undef M_TYPE
#undef M_PARAMS

}



#endif
