
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

    typedef typename Base::NodeBase                                                 NodeBase;
    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;
    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::TreePath                                            TreePath;

    Key getRawKey(Int i) const
    {
        return Base::getKey(i);
    }

    Accumulator getRawKeys() const
    {
        return Base::getKeys();
    }

    Key getKey(Int i) const
    {
        return me()->getRawKey(i) + me()->prefixes()[i];
    }

    Accumulator getKeys() const
    {
        return me()->getRawKeys() + me()->prefixes();
    }

//    bool nextKey()
//    {
//        if (!me()->isEnd())
//        {
//          Accumulator keys = me()->getRawKeys();
//          me()->prefix()   += keys;
//
//          bool        has_next    = Base::nextKey();
//
//            return has_next;
//        }
//        else {
//            return false;
//        }
//    }
//
//    bool prevKey()
//    {
//      if (!me()->isBegin())
//      {
//          bool result = Base::prevKey();
//
//          if (result)
//          {
//              Accumulator keys = me()->getRawKeys();
//              me()->prefix() -= keys;
//          }
//          else
//          {
//              me()->prefix().clear();
//          }
//
//          return result;
//      }
//      else {
//          return false;
//      }
//    }

    void ComputePrefix(Accumulator& pfx)
    {
        compute_base(pfx);
    }


    void ComputeBase()
    {
//        if (!me()->isEmpty())
//        {
//          me()->prefix().clear();
//
//            compute_base(me()->prefix());
//        }
    }

    void dumpKeys(ostream& out)
    {
        Base::dumpKeys(out);

        out<<"Prefix:  "<<me()->prefix()<<endl;
    }


    void init()
    {
        ComputeBase();
        Base::init();
    }

private:
    
    void compute_base(Accumulator& accum)
    {
        TreePath&   path0 = me()->path();
        Int         idx   = me()->key_idx();

        for (Int c = 0; c < path0.getSize(); c++)
        {
            me()->model().sumKeys(path0[c].node(), 0, idx, accum);
            idx = path0[c].parent_idx();
        }
    }


MEMORIA_ITERATOR_PART_END

}


#endif
