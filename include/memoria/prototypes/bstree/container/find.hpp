
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_VECTOR_MODEL_FIND_HPP
#define	_MEMORIA_MODELS_VECTOR_MODEL_FIND_HPP


#include <memoria/prototypes/bstree/names.hpp>
#include <memoria/core/container/container.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bstree::FindName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                              Allocator;

    typedef typename Allocator::Page                                              Page;
    typedef typename Page::ID                                                   ID;
    typedef typename Allocator::Transaction                                       Transaction;

    typedef typename Types::NodeBase                                            NodeBase;
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

        typename MyType::SearchModeDefault::Enum search_mode_;
        Key prefix_;
        Key current_prefix_;
        int type_;

        CompareBase(typename MyType::SearchModeDefault::Enum search_mode, Int type = LT): search_mode_(search_mode), prefix_(0), current_prefix_(0) , type_(type){}

        template <typename IteratorType>
        void SetupIterator(IteratorType &iter)
        {
            iter.prefix(0) = prefix_;
        }

        void AdjustKey(Key& key) {
            key -= current_prefix_;
        }

        typename MyType::SearchModeDefault::Enum search_mode() const {
        	return search_mode_;
        }
    };

    struct Compare: public CompareBase {

        Compare(typename MyType::SearchModeDefault::Enum search_mode, Int type): CompareBase(search_mode, type) {}

        template <typename Node>
        Int Find(Node* node, Int key_num, Key key)
        {
            Key& prefix = CompareBase::prefix_;
            Key& current_prefix = CompareBase::current_prefix_;
            current_prefix = 0;

            Int idx;

            if (CompareBase::type_ == CompareBase::LT)
            {
                idx = node->map().FindLTS(key_num, key, current_prefix);
            }
            else
            {
                idx = node->map().FindLES(key_num, key, current_prefix);
            }
            
            if (idx == -1 && node->children_count() > 0)
            {
                Key tmp;
                
                if (node->is_leaf())
                {
                    tmp = node->map().max_key(key_num);
                }
                else {
                    tmp = node->map().max_key(key_num) - node->map().key(key_num, node->children_count() - 1);
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
        CompareLT(typename MyType::SearchModeDefault::Enum search_mode): Compare(search_mode, CompareBase::LT) {}
    };

    struct CompareLE: public Compare
    {
        CompareLE(typename MyType::SearchModeDefault::Enum search_mode): Compare(search_mode, CompareBase::LE) {}
    };

    Iterator FindLT(Key key, Int key_num, bool for_insert)
    {
        return me()->template _find<CompareLT>(key, key_num, for_insert ? MyType::SearchModeDefault::LAST : MyType::SearchModeDefault::NONE);
    }

    Iterator FindLE(Key key, Int key_num, bool for_insert)
    {
        return me()->template _find<CompareLE>(key, key_num, for_insert ? MyType::SearchModeDefault::LAST : MyType::SearchModeDefault::NONE);
    }


MEMORIA_CONTAINER_PART_END

}


#endif
