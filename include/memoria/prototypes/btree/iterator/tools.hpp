
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

    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::Container                                                Container;
    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;

    NodeBase* GetParent(NodeBase *node)
    {
        return me_.model().GetParent(node);
    }

    Int GetChildrenCount(NodeBase *node)
    {
        return me_.model().GetChildrenCount(node);
    }

    NodeBase *GetChild(NodeBase *node, Int idx) {
        return me_.model().GetChild(node, idx);
    }

    NodeBase* GetLastChild(NodeBase *node)
    {
        return me_.model().GetLastChild(node);
    }

    Value GetData() {
        return me_.model().GetLeafData(me_.page(), me_.key_idx());
    }

    Key GetKey(Int keyNum) {
        return Container::GetKey(me_.page(), keyNum, me_.key_idx());
    }

MEMORIA_ITERATOR_PART_END

}

#endif
