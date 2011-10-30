
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


    typedef typename Base::NodeBase                                             NodeBase;
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
        Iterator i = me_.FindLE(key, c, false);
        if (!i.IsEnd())
        {
            NodeBase* page = i.page();
            BigInt keys = i.key_idx();

            while (!page->is_root())
            {
                Int pidx = page->parent_idx();
                page = me_.GetParent(page);

                for (Int c = 0; c < pidx; c++)
                {
                    NodeBase* child = me_.GetChild(page, c);
                    keys += child->counters().key_count();
                }
            }

            return keys;
        }
        else {
            return -1;
        }
    }

    Iterator GetByIndex(BigInt index)
    {
        NodeBase *node = me_.GetRoot();
        if (node != NULL)
        {
            BigInt keys = 0;
            while (!node->is_leaf())
            {
                Int size = me_.GetChildrenCount(node);

                Int idx = -1;

                for (Int c = 0; c < size; c++)
                {
                    NodeBase* child = me_.GetChild(node, c);
                    BigInt count = child->counters().key_count();

                    if (index < keys + count)
                    {
                        idx = c;
                        break;
                    }
                    else {
                        keys += count;
                    }
                }

                if (idx < 0)
                {
                    return Iterator(me_);
                }
                else {
                    node = me_.GetChild(node, idx);
                }
            }

            return Iterator(node, index - keys, me_, true);
        }
        else {
            return Iterator(me_);
        }
    }
    
    virtual BigInt GetKeyIndex(BigInt key, Int i = 0)
    {
    	return me_.KeyIndex(key, i);
    }



    virtual bool Get(BigInt index, BigInt& key, BigInt& value)
    {
    	Iterator i = me_.GetByIndex(index);
    	if (i.IsEnd())
    	{
    		return false;
    	}
    	else {
    		//i.GetValue(value);
    		value = ConvertToHelper<Value, ID>::cvt(i.GetData());
    		key = i.GetKey(0);

    		return true;
    	}
    }


MEMORIA_CONTAINER_PART_END

}


#endif
