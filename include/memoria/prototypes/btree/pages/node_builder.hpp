
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_BUILDER_HPP
#define _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_BUILDER_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/prototypes/btree/pages/node_builder_intrnl.hpp>

namespace memoria    {
namespace btree      {

template <
        typename Types,
        typename KeyTypesList
>
struct NodeTLBuilder {

    typedef typename NodeTLBuilderTool <
                                Types,
                                KeyTypesList
    >::NodeTypesList                                                            List;

};


template <
        typename Types,
        typename Key
>
struct NodeTLBuilder<
        Types,
        VTL<Key>
> {
    typedef typename AddTypesQuadToList <
                                Types,
                                VTL<>,
                                ANY_LEVEL,
                                true
    >::List                                                                     List;
};


}
}

#endif
