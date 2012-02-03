
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_ITERATOR_TOOLS_HPP
#define _MEMORIA_MODELS_IDX_MAP_ITERATOR_TOOLS_HPP

#include <iostream>

#include <memoria/core/types/types.hpp>

#include <memoria/containers/idx_map/names.hpp>
#include <memoria/core/container/iterator.hpp>



namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bstree::IteratorToolsName)

    typedef typename Base::NodeBase                                             	NodeBase;
	typedef typename Base::NodeBaseG                                             	NodeBaseG;
    typedef typename Base::Container                                                Container;
    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;
    typedef typename Container::Allocator											Allocator;

    static const Int Indexes = Container::Indexes;


    Key GetRawKey(Int i)
    {
        return Base::GetKey(i);
    }

    Key GetKey(Int i)
    {
        return me()->GetRawKey(i) + me()->prefix(i);
    }

    void AddKey(Int key_num, const Key& key)
    {
    	Key keys[Indexes];

    	for (Key& k: keys) k = 0;

    	keys[key_num] = key;

    	me()->model().AddKeysUp(me()->page(), me()->key_idx(), keys);
    }

    bool NextKey()
    {
        if (!me()->IsEnd())
        {
            for (Int c = 0; c < Indexes; c++)
            {
                me()->prefix(c) += me()->GetRawKey(c);
            }
            return Base::NextKey();
        }
        else {
            return false;
        }
    }

    bool PrevKey()
    {
    	if (!me()->IsStart())
    	{
    		bool result = Base::PrevKey();

    		if (result)
    		{
    			for (Int c = 0; c < Indexes; c++)
    			{
    				me()->prefix(c) -= me()->GetRawKey(c);
    			}
    		}
    		else
    		{
    			//FIXME: this might hide errors
    			for (Int c = 0; c < Indexes; c++)
    			{
    				me()->prefix(c) = 0;
    			}
    		}

    		return result;
    	}
    	else {
    		return false;
    	}
    }


    void ComputeBase()
    {
        if (!me()->IsEmpty())
        {
        	for (Int c = 0; c < Indexes; c++)
        	{
        		me()->prefix(c) = 0;
        	}
            compute_base(&me()->prefix(0), me()->page(), me()->key_idx());
        }
    }


    //FIXME: NextLeaf/PrevLeaf don't work properly for this container (prefixes)

//
//    bool NextLeaf() {
//        if(Base::NextLeaf()) {
//            ComputeBase();
//            return true;
//        }
//        return false;
//    }
//
//    bool PrevLeaf() {
//        if(Base::PrevLeaf()) {
//            ComputeBase();
//            return true;
//        }
//        return false;
//    }

    void Init() {
        ComputeBase();
        Base::Init();
    }

private:
    
    void compute_base(Key* values, NodeBaseG& page, Int idx)
    {
        if (idx > 0) {
            me()->model().SumKeys(page, 0, idx, values);
        }
        
        if (!page->is_root())
        {
            Int parent_idx = page->parent_idx();
            NodeBaseG parent = me()->model().GetParent(page, Allocator::READ);
            compute_base(values, parent, parent_idx);
        }
    }


MEMORIA_ITERATOR_PART_END

}


#endif
