
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_MODEL_REMOVE_HPP
#define	_MEMORIA_MODELS_IDX_MAP_MODEL_REMOVE_HPP


#include <memoria/prototypes/btree/pages/tools.hpp>
#include <memoria/containers/idx_map/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::idx_map::RemoveName)

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


    bool RemoveAllEntries(Key from, Key to, Int i, Int type) {
        if (to >= from)
        {
            Iterator ifrom  = type == memoria::vapi::IdxMap::LE ? me_.FindLE(from, i, true)  : me_.FindLT(from, i, false);
            Iterator ito    = type == memoria::vapi::IdxMap::LE ? me_.FindLE(to, i, true)    : me_.FindLT(to, i, false);

            MEMORIA_TRACE(me_, "RemoveAllEntries", from, to, type, ito.page()->id());

            if (!(ifrom.IsEmpty() || ifrom.IsEnd())) {
                me_.RemoveEntries(ifrom, ito);
            }
        }
        return false;
    }


MEMORIA_CONTAINER_PART_END

}


#endif
