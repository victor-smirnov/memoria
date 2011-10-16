
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_VECTOR_MODEL_FIND_HPP
#define	_MEMORIA_MODELS_VECTOR_MODEL_FIND_HPP


#include <memoria/prototypes/btree/pages/tools.hpp>
#include <memoria/containers/idx_map/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::idx_map::FindName)

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

    struct CompareBase {

        enum {LT, LE};

        bool for_insert_;
        Key prefix_;
        Key current_prefix_;
        int type_;

        CompareBase(bool for_insert, Int type = LT): for_insert_(for_insert), prefix_(0), type_(type) {}

        template <typename IteratorType>
        void SetupIterator(IteratorType &iter) {
            iter.prefix(0) = prefix_;
        }

        void AdjustKey(Key& key) {
            key -= current_prefix_;
        }
    };

    struct Compare: public CompareBase {

        Compare(bool for_insert, Int type): CompareBase(for_insert, type) {}

        template <typename Node>
        Int Find(Node* node, Int c, Key key)
        {
            Key& prefix = CompareBase::prefix_;
            Key& current_prefix = CompareBase::current_prefix_;
            current_prefix = 0;

            Int idx;

            if (CompareBase::type_ == CompareBase::LT)
            {
                idx = node->map().FindLTS(c, key, current_prefix);
            }
            else {
                idx = node->map().FindLES(c, key, current_prefix);
            }
            
            if (idx == -1 && node->map().size() > 0)
            {
                Key tmp;
                
                if (node->is_leaf())
                {
                    tmp = node->map().max_key(c);
                }
                else {
                    tmp = node->map().max_key(c) - node->map().key(c, node->map().size() - 1);
                }

                current_prefix += tmp;
            }

            prefix += current_prefix;

            return idx;
        }

        bool CompareMax(Key key, Key max_key)
        {
            return key >= max_key;
        }
    };

    struct CompareLT: public Compare
    {
        CompareLT(bool for_insert): Compare(for_insert, CompareBase::LT) {}
    };

    struct CompareLE: public Compare
    {
        CompareLE(bool for_insert): Compare(for_insert, CompareBase::LE) {}
    };

    Iterator FindLT(Key key, Int c, bool for_insert)
    {
        return me_.template _find<CompareLT>(key, c, for_insert);
    }

    Iterator FindLE(Key key, Int c, bool for_insert)
    {
        return me_.template _find<CompareLE>(key, c, for_insert);
    }

    bool IsFound(Iterator &iter, Key key, Int i)
    {
        if (!(iter.IsEnd() || iter.IsEmpty()))
        {
            if (iter.GetKey(i) == key)
            {
                return true;
            }
        }
        return false;
    }

MEMORIA_CONTAINER_PART_END

}


#endif
