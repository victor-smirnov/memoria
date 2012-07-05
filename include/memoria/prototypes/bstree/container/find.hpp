
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
        Int key_num_;

        CompareBase(Int key_num):
        	prefix_(0),
        	current_prefix_(0),
        	key_num_(key_num)
        {}

        template <typename IteratorType>
        void setupIterator(IteratorType &iter)
        {
        	iter.setupPrefix(prefix_, key_num_);
        }

        void AdjustKey(Key& key)
        {
            key -= current_prefix_;
        }

        template <typename Node>
        Int find(Node* node, Key key)
        {
            current_prefix_ = 0;

            Int idx = me()->findInMap(node, key, current_prefix_);
            
            if (idx == -1 && node->children_count() > 0)
            {
                Key tmp;
                
                if (node->is_leaf())
                {
                    tmp = node->map().max_key(key_num_);
                }
                else {
                    tmp = node->map().max_key(key_num_) - node->map().key(key_num_, node->children_count() - 1);
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

        Int key_num() const {
        	return key_num_;
        }
    };

    struct CompareLT: public CompareBase<CompareLT>
    {
    	typedef CompareBase<CompareLT> Base;

    	CompareLT(Int key_num): Base(key_num) {}

    	template<typename Node>
    	Int findInMap(Node* node, Key key, Key& prefix)
    	{
    		return node->map().findLTS(Base::key_num(), key, prefix);
    	}

    	template <typename Node>
    	bool IsKeyWithinRange(Node* node, Key key) const
    	{
    		return key < node->map().max_key(Base::key_num());
    	}
    };

    struct CompareLE: public CompareBase<CompareLE>
    {
    	typedef CompareBase<CompareLE> Base;

        CompareLE(Int key_num): Base(key_num) {}

        template<typename Node>
        Int findInMap(Node* node, Key key, Key& prefix)
        {
        	return node->map().findLES(Base::key_num(), key, prefix);
        }

        template <typename Node>
        bool IsKeyWithinRange(Node* node, Key key) const
        {
        	return key <= node->map().max_key(Base::key_num());
        }
    };

    Iterator findLT(Key key, Int key_num)
    {
        return me()->template _find<CompareLT>(key, key_num);
    }

    Iterator findLE(Key key, Int key_num)
    {
        return me()->template _find<CompareLE>(key, key_num);
    }


MEMORIA_CONTAINER_PART_END

}


#endif
