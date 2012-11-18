
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_BUILDER_HPP
#define _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_BUILDER_HPP

#include <memoria/prototypes/btree/pages/dispatchers/defs.hpp>
#include <memoria/prototypes/btree/pages/dispatchers/tools.hpp>
#include <memoria/prototypes/btree/pages/dispatchers/factory.hpp>
#include <memoria/prototypes/btree/pages/dispatchers/node_dispatcher.hpp>


namespace memoria    {
namespace btree      {


using memoria::TL;

template <
    typename Types
>
class BTreeDispatchers: public Types {

    typedef BTreeDispatchers<Types>                                         MyType;
protected:
    typedef typename Sorter<
                NodeDescriptorMetadata,
                LEVEL,
                true,
                typename Types::NodeList
    >::Result                                                                               NodeList_;

public:

    typedef typename NodeFilter<ValueOp<ROOT, EQ, bool, true>,  NodeList_>::Result          RootList;
    typedef typename NodeFilter<ValueOp<LEAF, EQ, bool, true>,  NodeList_>::Result          LeafList;
    typedef typename NodeFilter<ValueOp<LEAF, EQ, bool, false>, NodeList_>::Result          NonLeafList;
    typedef typename NodeFilter<ValueOp<ROOT, EQ, bool, false>, NodeList_>::Result          NonRootList;


    typedef typename FindNodeWithMaxLevelTool<true,  false, NodeList_>::Type                RootNode;
    typedef typename FindNodeWithMaxLevelTool<false, true,  NodeList_>::Type                LeafNode;
    typedef typename FindNodeWithMaxLevelTool<true,  true,  NodeList_>::Type                RootLeafNode;
    typedef typename FindNodeWithMaxLevelTool<false, false, NodeList_>::Type                Node;

    struct NodeTypesBase {
        typedef typename Types::NodeBase NodeBase;
    };

    struct AllTypes: NodeTypesBase {
        typedef NodeList_ List;
    };

    struct RootTypes: NodeTypesBase {
        typedef RootList List;
    };

    struct LeafTypes: NodeTypesBase {
        typedef LeafList List;
    };

    struct NonLeafTypes: NodeTypesBase {
        typedef NonLeafList List;
    };

    struct NonRootTypes: NodeTypesBase {
        typedef NonRootList List;
    };

    typedef NDT<AllTypes>                                   NodeDispatcher;
    typedef NDT<RootTypes>                                  RootDispatcher;
    typedef NDT<LeafTypes>                                  LeafDispatcher;
    typedef NDT<NonLeafTypes>                               NonLeafDispatcher;
    typedef NDT<NonRootTypes>                               NonRootDispatcher;


    typedef typename Node2NodeMapTool<NodeList_, true>::Map                                 Root2NodeMap;
    typedef typename Node2NodeMapTool<NodeList_, false>::Map                                Node2RootMap;

    typedef NodeFactoryTool<NodeList_, MyType>                                              NodeFactory;
};

}
}

#endif
