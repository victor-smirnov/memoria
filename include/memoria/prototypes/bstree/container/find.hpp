
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

    template <typename CmpType>
    struct CompareBase {

        Key prefix_;
        Key current_prefix_;

        CompareBase():
        	prefix_(0),
        	current_prefix_(0)
        {}

        template <typename IteratorType>
        void SetupIterator(IteratorType &iter){}

        void AdjustKey(Key& key)
        {
            key -= current_prefix_;
        }

        template <typename Node>
        Int Find(Node* node, Int key_num, Key key)
        {
            current_prefix_ = 0;

            Int idx = me()->FindInMap(node, key_num, key, current_prefix_);
            
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

                current_prefix_ += tmp;
            }

            prefix_ += current_prefix_;

            return idx;
        }

        bool CompareMax(Key key, Key max_key)
        {
            return key >= max_key;
        }

        const CmpType* me() const {
        	return static_cast<const CmpType*>(this);
        }

        CmpType* me() {
        	return static_cast<CmpType*>(this);
        }
    };

    struct CompareLT: public CompareBase<CompareLT>
    {
    	CompareLT(): CompareBase<CompareLT>() {}

    	template<typename Node>
    	Int FindInMap(Node* node, Int key_num, Key key, Key& prefix)
    	{
    		return node->map().FindLTS(key_num, key, prefix);
    	}

    	template <typename Node>
    	bool IsKeyWithinRange(Node* node, Int block_num, Key key) const
    	{
    		return key < node->map().max_key(block_num);
    	}
    };

    struct CompareLE: public CompareBase<CompareLE>
    {
        CompareLE(): CompareBase<CompareLE>() {}

        template<typename Node>
        Int FindInMap(Node* node, Int key_num, Key key, Key& prefix)
        {
        	return node->map().FindLES(key_num, key, prefix);
        }

        template <typename Node>
        bool IsKeyWithinRange(Node* node, Int block_num, Key key) const
        {
        	return key <= node->map().max_key(block_num);
        }
    };

    Iterator FindLT(Key key, Int key_num)
    {
        return me()->template _find<CompareLT>(key, key_num);
    }

    Iterator FindLE(Key key, Int key_num)
    {
        return me()->template _find<CompareLE>(key, key_num);
    }


MEMORIA_CONTAINER_PART_END

}


#endif
