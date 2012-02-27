
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
#define	_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP


#include <memoria/containers/kvmap/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {


MEMORIA_CONTAINER_PART_BEGIN(memoria::models::kvmap::FindName)

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
    typedef typename Base::Value                                                Value;

    static const Int Indexes                                                    = Types::Indexes;

    struct CompareLE {
    	typename MyType::SearchModeDefault::Enum search_mode_;
        Key prefix_;
        CompareLE(typename MyType::SearchModeDefault::Enum search_mode): search_mode_(search_mode), prefix_(0) {}

        template <typename Node>
        Int Find(Node* node, Int c, Key key) {
            return node->map().FindLE(c, key);
        }

        bool CompareMax(Key key, Key max_key) {
            return key > max_key;
        }

        template <typename IteratorType>
        void SetupIterator(IteratorType &iter) {}

        void AdjustKey(Key& key) {}

        typename MyType::SearchModeDefault::Enum search_mode() const {
        	return search_mode_;
        }
    };

    Iterator FindLE(Key key, int i, bool for_insert)
    {
        return me()->template _find<CompareLE>(key, i, for_insert);
    }

MEMORIA_CONTAINER_PART_END

}


#endif	//_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
