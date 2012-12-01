
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_BUILDER_INTRNL_HPP
#define _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_BUILDER_INTRNL_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/prototypes/btree/pages/node_factory.hpp>

namespace memoria    {
namespace btree      {

template <
        typename Types,
        typename ResultList,
        Int Level,
        bool All
>
class AddTypesQuadToList;




template <
        typename Types,
        typename ResultList
>
class AddTypesQuadToList <
        Types,
        ResultList,
        ANY_LEVEL,
        true
> {
    static const BigInt Level = ANY_LEVEL;

public:
    typedef typename AppendTool<
                        VTL<
                            NodePage<typename Types::template LeafTypes<Level> >,
                            NodePage<typename Types::template RootLeafTypes<Level> >,
                            NodePage<typename Types::template RootTypes<Level> >,
                            NodePage<typename Types::template InternalTypes<Level> >
                        >,
                        ResultList
    >::Result                                                                   List;
};


template <
        typename Types,
        typename ResultList,
        bool     All
>
class AddTypesQuadToList<
        Types,
        ResultList,
        0,
        All
> {
    static const Int Level = 0;


public:
    typedef typename AppendTool<
                        VTL<
                            NodePage<typename Types::template LeafTypes<Level> >,
                            NodePage<typename Types::template RootLeafTypes<Level> >
                        >,
                        ResultList
    >::Result                                                                   List;
};

template <
        typename Types,
        typename ResultList,
        Int Level,
        bool All
>
class AddTypesQuadToList {

public:
    typedef typename AppendTool<
                        VTL<
                            NodePage<typename Types::template RootTypes<Level> >,
                            NodePage<typename Types::template InternalTypes<Level> >
                        >,
                        ResultList
    >::Result                                                                   List;
};








template <
        typename Types,
        typename KeysList,
        typename ResultList = VTL<>,
        Int Level = 0
>
class NodeTLBuilderTool;


template <
        typename Types,
        typename Head,
        typename ... Tail,
        typename ResultList,
        Int Level
>
class NodeTLBuilderTool<
        Types,
        VTL<Head, Tail...>,
        ResultList,
        Level
> {
    typedef Head                                                                Key;
    
    typedef typename AddTypesQuadToList<
                        Types,
                        ResultList,
                        Level,
                        false
    >::List                                                                     List;

public:

    typedef typename NodeTLBuilderTool<
                        Types,
                        VTL<Tail...>,
                        List,
                        Level + 1
    >::NodeTypesList                                                            NodeTypesList;
};


template <
        typename Types,
        typename Head,
        typename ResultList,
        Int Level
>
class NodeTLBuilderTool<
        Types,
        VTL<Head>,
        ResultList,
        Level
> {
    typedef Head                                                                Key;

    typedef typename AddTypesQuadToList<
                        Types,
                        ResultList,
                        ANY_LEVEL,
                        false
    >::List                                                                     List;

public:

    typedef List                                                                NodeTypesList;
};

}
}

#endif
