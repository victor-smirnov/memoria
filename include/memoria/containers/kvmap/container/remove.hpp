
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_KVMAP_MODEL_REMOVE_HPP
#define	_MEMORIA_MODELS_KVMAP_MODEL_REMOVE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/kvmap/names.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::kvmap::RemoveName)

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

    bool RemoveByKey(Key key) {
        return RemoveByKey(key, 0);
    }

    bool RemoveByKey(Key key, Int c) {
        Iterator i = me_.FindLE(key, c, false);
        if (i.IsFound()) {
            if (i.GetKey(c) == key) {
                me_.RemoveEntry(i);
                return true;
            }
        }
        return false;
    }

MEMORIA_CONTAINER_PART_END

}

#endif
