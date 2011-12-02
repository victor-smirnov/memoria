
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


MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::itree::IteratorToolsName)

    typedef typename Base::NodeBase                                             	NodeBase;
	typedef typename Base::NodeBaseG                                             	NodeBaseG;
    typedef typename Base::Container                                                Container;
    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;

    static const Int Indexes = Container::Indexes;



    IterPart(MyType &me): Base(me), me_(me)
    {

    }

    Key GetRawKey(Int i)
    {
        return Base::GetKey(i);
    }

    Key GetKey(Int i)
    {
        return me_.GetRawKey(i) + me_.prefix(i);
    }

    bool NextKey()
    {
        if (!me_.IsEnd())
        {
            for (Int c = 0; c < Indexes; c++)
            {
                me_.prefix(c) += me_.GetRawKey(c);
            }
            return Base::NextKey();
        }
        else {
            return false;
        }
    }

    bool PrevKey()
    {
    	if (!me_.IsStart())
    	{
    		bool result = Base::PrevKey();

    		if (result)
    		{
    			for (Int c = 0; c < Indexes; c++)
    			{
    				me_.prefix(c) -= me_.GetRawKey(c);
    			}
    		}
    		else
    		{
    			//FIXME: this might hide errors
    			for (Int c = 0; c < Indexes; c++)
    			{
    				me_.prefix(c) = 0;
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
        if (!me_.IsEmpty())
        {
        	for (Int c = 0; c < Indexes; c++)
        	{
        		me_.prefix(c) = 0;
        	}
            compute_base(&me_.prefix(0), me_.page(), me_.key_idx());
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
            me_.model().SumKeys(page, 0, idx, values);
        }
        
        if (!page->is_root())
        {
            Int parent_idx = page->parent_idx();
            NodeBaseG parent = me_.model().GetParent(page);
            compute_base(values, parent, parent_idx);
        }
    }


MEMORIA_ITERATOR_PART_END

}


#endif
