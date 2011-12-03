
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_KVMAP_MODEL_MAP_API_HPP
#define _MEMORIA_MODELS_KVMAP_MODEL_MAP_API_HPP

#include <memoria/containers/kvmap/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {


template <typename TargetType, typename IDType>
struct ConvertHelper1 {
	static BigInt cvt(TargetType type) {
		return (BigInt)type;
	}
};

template <typename IDType>
struct ConvertHelper1<IDType, IDType> {
	static BigInt cvt(IDType type) {
		return (BigInt)type.value();
	}
};


MEMORIA_CONTAINER_PART_BEGIN(memoria::models::kvmap::MapApiName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                              Allocator;

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

//    typedef Map::Pair Pair;

    bool Get(Key key, Value& value, Int idx = 0)
    {
    	return me()->GetValue(key, idx, value);
    }

    bool Get1(Key key, Value& value)
    {
    	return me()->GetValue(key, 0, value);
    }

    void Put(Key key, const Value& value)
    {
        me()->SetValueForKey(key, value);
    }

    bool Remove(Key key)
    {
        return me()->RemoveByKey(key);
    }

MEMORIA_CONTAINER_PART_END

}

#endif
