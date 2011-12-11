
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_API_HPP
#define _MEMORIA_PROTOTYPES_BTREE_MODEL_API_HPP

#include <memoria/prototypes/btree/names.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::ApiName)

    typedef typename Base::Name                                                 Name;
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                              Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;


    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                             NodeBaseG;
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

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::ApiName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}


#endif
