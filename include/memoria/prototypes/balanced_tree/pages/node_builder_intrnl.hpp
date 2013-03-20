
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_BUILDER_INTRNL_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_BUILDER_INTRNL_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/prototypes/balanced_tree/pages/node_factory.hpp>

namespace memoria    	{
namespace balanced_tree {

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
                        TypeList<
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
                        TypeList<
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
                        TypeList<
                            NodePage<typename Types::template RootTypes<Level> >,
                            NodePage<typename Types::template InternalTypes<Level> >
                        >,
                        ResultList
    >::Result                                                                   List;
};








template <
        typename Types,
        typename KeysList,
        typename ResultList = TypeList<>,
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
        TypeList<Head, Tail...>,
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
                        TypeList<Tail...>,
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
        TypeList<Head>,
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
