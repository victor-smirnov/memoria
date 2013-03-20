
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_BUILDER_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_BUILDER_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/prototypes/balanced_tree/pages/node_builder_intrnl.hpp>

namespace memoria    	{
namespace balanced_tree	{

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
        TypeList<Key>
> {
    typedef typename AddTypesQuadToList <
                                Types,
                                TypeList<>,
                                ANY_LEVEL,
                                true
    >::List                                                                     List;
};


}
}

#endif
