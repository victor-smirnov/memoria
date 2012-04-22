
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_ITREE_ITERATOR_BASE_H
#define __MEMORIA_PROTOTYPES_ITREE_ITERATOR_BASE_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/bstree/names.hpp>
#include <memoria/prototypes/bstree/macros.hpp>

#include <memoria/core/tools/hash.hpp>

namespace memoria {

using namespace memoria::bstree;


MEMORIA_BSTREE_ITERATOR_BASE_CLASS_BEGIN(ITreeIteratorBase)
public:

	typedef typename Base::Container::Key                                        	 	Key;
    typedef typename Base::Container::NodeBase											NodeBase;
    typedef typename Base::Container::Accumulator										Accumulator;
    typedef typename Base::Container::TreePath											TreePath;

    const Key prefix(Int i) const
    {
    	return get_prefix(i);
    }

    const Accumulator prefix() const
    {
    	return get_prefixes();
    }

private:

    Accumulator get_prefixes() const
    {
    	Accumulator accum;

    	const TreePath& path0 = me()->path();
    	Int 			idx   = me()->key_idx();

    	for (Int c = 0; c < path0.GetSize(); c++)
    	{
    		me()->model().SumKeys(path0[c].node(), 0, idx, accum);
    		idx = path0[c].parent_idx();
    	}

    	return accum;
    }

    Key get_prefix(Int block_num) const
    {
    	Key accum = 0;

    	const TreePath& path0 = me()->path();
    	Int 			idx   = me()->key_idx();

    	for (Int c = 0; c < path0.GetSize(); c++)
    	{
    		me()->model().SumKeys(path0[c].node(), block_num, 0, idx, accum);
    		idx = path0[c].parent_idx();
    	}

    	return accum;
    }

MEMORIA_BSTREE_ITERATOR_BASE_CLASS_END

}



#endif
