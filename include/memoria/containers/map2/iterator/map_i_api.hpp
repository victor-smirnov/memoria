
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP2_ITERATOR_API_HPP
#define _MEMORIA_MODELS_IDX_MAP2_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map2/map_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::map2::ItrApiName)

	typedef typename Base::Allocator                                            Allocator;
	typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::TreePath                                             TreePath;

	typedef typename Base::Container::Value                                     Value;
	typedef typename Base::Container::Key                                       Key;
	typedef typename Base::Container::Element                                   Element;
	typedef typename Base::Container::Accumulator                               Accumulator;
	typedef typename Base::Container                                            Container;


    void ComputePrefix(Accumulator& accum)
    {
    	TreePath&   path0 = me()->path();
    	Int         idx   = me()->key_idx();

    	me()->model().sumLeafKeys(path0[0].node(), 0, idx, accum);

    	for (Int c = 1; c < path0.getSize(); c++)
    	{
    		idx = path0[c - 1].parent_idx();
    		me()->model().sumKeys(path0[c].node(), 0, idx, accum);
    	}
    }

MEMORIA_ITERATOR_PART_END

}

#endif
