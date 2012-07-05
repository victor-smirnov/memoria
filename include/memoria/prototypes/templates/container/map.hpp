
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_MAP_MODEL_API_HPP
#define _MEMORIA_MODELS_MAP_MODEL_API_HPP


#include <memoria/core/container/container.hpp>
#include <memoria/prototypes/templates/names.hpp>


namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::MapName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;


    typedef typename Base::NodeBase                                             NodeBase;
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

    void setValueForKey(Key key, const Value &value) {
        MEMORIA_MODEL_METHOD_IS_NOT_IMPLEMENTED();
    }
    
    bool RemoveByKey(Key key) {
        MEMORIA_MODEL_METHOD_IS_NOT_IMPLEMENTED();
    }

    bool getValue(Key key, Int c, Value &value) {
        Iterator i = me()->findLE(key, c, false);
        if (i.IsEnd()) {
            return false;
        }
        else {
            Key k = i.getKey(c);
            if (k == key) {
                value = i.getValue();
                return true;
            }
            else {
                return false;
            }
        }
    }

    
    

MEMORIA_CONTAINER_PART_END

}

#endif
