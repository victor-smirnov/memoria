
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_TOOLS_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_TOOLS_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/btree/names.hpp>

namespace memoria    {

using namespace memoria::btree;

MEMORIA_ITERATOR_PART_BEGIN(memoria::btree::IteratorToolsName)


    typedef typename Base::Container                                                Container;
	typedef typename Container::Types::NodeBase                                     NodeBase;
	typedef typename Container::Types::NodeBaseG                                    NodeBaseG;

    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;

    NodeBaseG GetParent(NodeBase *node)
    {
        return me()->model().GetParent(node);
    }

    Int GetChildrenCount(NodeBase *node)
    {
        return me()->model().GetChildrenCount(node);
    }

    NodeBaseG GetChild(NodeBase *node, Int idx)
    {
        return me()->model().GetChild(node, idx);
    }

    NodeBaseG GetLastChild(NodeBase *node)
    {
        return me()->model().GetLastChild(node);
    }

    Value GetData() {
        return me()->model().GetLeafData(me()->page(), me()->key_idx());
    }

    Key GetKey(Int keyNum) {
        return Container::GetKey(me()->page(), keyNum, me()->key_idx());
    }

MEMORIA_ITERATOR_PART_END

}

#endif
