
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_STUBS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_STUBS_HPP



#include <iostream>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::StubsName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                              Allocator;

    typedef typename Allocator::Page                                              Page;
    typedef typename Page::ID                                                   ID;
    typedef typename Allocator::Transaction                                       Transaction;

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
    typedef typename Base::Value                                                Value;

    static const Int Indexes                                                    = Types::Indexes;


    void PostSplit(NodeBase *one, NodeBase *two, Int from) {
//        MEMORIA_TRACE(me_, "Default PostSplit", one, two, from);
    }

    void PreMerge(NodeBase *one, NodeBase *two) {
//        MEMORIA_TRACE(me_, "Default PreMerge", one, two);
    }

//    template <typename Node>
//    void SetupKeysFromChild(Node *node, Int idx, const Key* keys) {
//        for (Int c = 0; c < Indexes; c++)
//        {
//            node->map().key(c, idx) = keys[c];
//        }
//    }

    
//    void ShiftKeysFromChild(NodeBase *node, Key* keys) {}

MEMORIA_CONTAINER_PART_END

}

#endif

