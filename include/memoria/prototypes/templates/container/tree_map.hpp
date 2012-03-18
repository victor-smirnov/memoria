
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_TREE_MAP_MODEL_API_HPP
#define _MEMORIA_MODELS_TREE_MAP_MODEL_API_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/prototypes/templates/names.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::TreeMapName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;


    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
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

    BigInt KeyIndex(Key key, Int c)
    {
//        Iterator i = me()->FindLE(key, c, false);
//        if (!i.IsEnd())
//        {
//            NodeBaseG page = i.page();
//            BigInt keys = i.key_idx();
//
//            while (!page->is_root())
//            {
//                Int pidx = page->parent_idx();
//                page = me()->GetParent(i.path(), page);
//
//                for (Int c = 0; c < pidx; c++)
//                {
//                    NodeBaseG child = me()->GetChild(page, c, Allocator::READ);
//                    keys += child->counters().key_count();
//                }
//            }
//
//            return keys;
//        }
//        else {
//            return -1;
//        }
    	return 0;
    }

    Iterator GetByIndex(BigInt index)
    {
//        NodeBaseG node = me()->GetRoot(Allocator::READ);
//        if (node.is_set())
//        {
//        	Iterator iter(*me(), node->level());
//
//        	BigInt keys = 0;
//            while (!node->is_leaf())
//            {
//                Int size = node->children_count();
//
//                Int idx = -1;
//
//                for (Int c = 0; c < size; c++)
//                {
//                    NodeBaseG child = me()->GetChild(node, c, Allocator::READ);
//                    BigInt count = child->counters().key_count();
//
//                    if (index < keys + count)
//                    {
//                        idx = c;
//                        break;
//                    }
//                    else {
//                        keys += count;
//                    }
//                }
//
//                if (idx < 0)
//                {
//                    return Iterator(*me());
//                }
//                else {
//                    node = me()->GetChild(node, idx, Allocator::READ);
//                }
//            }
//
//            iter.Init();
//
//            return iter;
//        }
//        else {
//            return Iterator(*me());
//        }

    	return Iterator(*me());
    }
    
    virtual BigInt GetKeyIndex(BigInt key, Int i = 0)
    {
    	return me()->KeyIndex(key, i);
    }



    // FIXME: value transfer is not yet implemented
    virtual bool Get(BigInt index, BigInt& key, BigInt& value)
    {
    	Iterator i = me()->GetByIndex(index);
    	if (i.IsEnd())
    	{
    		return false;
    	}
    	else {

    		value = ConvertToHelper<Value, ID>::cvt(i.GetData());
    		key = i.GetKey(0);

    		return true;
    	}
    }


MEMORIA_CONTAINER_PART_END

}


#endif
