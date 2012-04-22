
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_ITERATOR_API1_HPP
#define _MEMORIA_MODELS_IDX_MAP_ITERATOR_API1_HPP

#include <iostream>

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map/names.hpp>
#include <memoria/core/container/iterator.hpp>



namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bstree::ItrApiName)

    typedef typename Base::NodeBase                                             	NodeBase;
	typedef typename Base::NodeBaseG                                             	NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;
    typedef typename Container::Allocator											Allocator;
    typedef typename Container::Accumulator											Accumulator;
    typedef typename Container::TreePath											TreePath;

    Key GetRawKey(Int i) const
    {
        return Base::GetKey(i);
    }

    Accumulator GetRawKeys() const
    {
    	return Base::GetKeys();
    }

    Key GetKey(Int i) const
    {
        return me()->GetRawKey(i) + me()->prefix(i);
    }

    Accumulator GetKeys() const
    {
    	return me()->GetRawKeys() + me()->prefix();
    }

//    bool NextKey()
//    {
//        if (!me()->IsEnd())
//        {
//        	Accumulator keys = me()->GetRawKeys();
//        	me()->prefix() 	 += keys;
//
//        	bool 		has_next 	= Base::NextKey();
//
//            return has_next;
//        }
//        else {
//            return false;
//        }
//    }
//
//    bool PrevKey()
//    {
//    	if (!me()->IsBegin())
//    	{
//    		bool result = Base::PrevKey();
//
//    		if (result)
//    		{
//    			Accumulator keys = me()->GetRawKeys();
//    			me()->prefix() -= keys;
//    		}
//    		else
//    		{
//    			me()->prefix().Clear();
//    		}
//
//    		return result;
//    	}
//    	else {
//    		return false;
//    	}
//    }

    void ComputePrefix(Accumulator& pfx)
    {
    	compute_base(pfx);
    }


    void ComputeBase()
    {
//        if (!me()->IsEmpty())
//        {
//        	me()->prefix().Clear();
//
//            compute_base(me()->prefix());
//        }
    }

    void DumpKeys(ostream& out)
    {
    	Base::DumpKeys(out);

    	out<<"Prefix:  "<<me()->prefix()<<endl;
    }


    void Init()
    {
        ComputeBase();
        Base::Init();
    }

private:
    
    void compute_base(Accumulator& accum)
    {
    	TreePath& 	path0 = me()->path();
    	Int 		idx   = me()->key_idx();

        for (Int c = 0; c < path0.GetSize(); c++)
        {
        	me()->model().SumKeys(path0[c].node(), 0, idx, accum);
        	idx = path0[c].parent_idx();
        }
    }


MEMORIA_ITERATOR_PART_END

}


#endif