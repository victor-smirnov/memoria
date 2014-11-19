
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_LIST_BUILDER_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_LIST_BUILDER_HPP

#include <memoria/core/types/typelist.hpp>


namespace memoria       {
namespace bt    {

template <
    template <typename> class TreeNode,
    typename Types
>
class NodePageAdaptor;



template<
    template <typename> class Type
>
struct LeafNodeTypes {
    template <typename Types>
    using AllTypesList = TypeList<
            NodePageAdaptor<Type, Types>
    >;

    template <typename Types>
    using LeafTypesList = TypeList<
            NodePageAdaptor<Type, Types>
    >;

    template <typename Types>
    using NonLeafTypesList = TypeList<>;
};





template<
    template <typename> class Type
>
struct NonLeafNodeTypes {
    template <typename Types>
    using AllTypesList = TypeList<
            NodePageAdaptor<Type, Types>
    >;

    template <typename Types>
    using LeafTypesList = TypeList<>;

    template <typename Types>
    using NonLeafTypesList = TypeList<
            NodePageAdaptor<Type, Types>
    >;
};



template<
    template <typename> class NodeType
>
struct TreeNodeType {
    template <typename Types>
    using Type = NodePageAdaptor<NodeType, Types>;
};


template <typename Types, typename NodeTypes> struct NodeTypeListBuilder;


template <typename Types, typename Head, typename... Tail>
struct NodeTypeListBuilder<Types, TypeList<Head, Tail...>> {

    typedef typename MergeLists<
            typename Head::template AllTypesList<Types>,
            typename NodeTypeListBuilder<Types, TypeList<Tail...>>::AllTypesList
    >::Result                                                                   AllTypesList;


    typedef typename MergeLists<
            typename Head::template LeafTypesList<Types>,
            typename NodeTypeListBuilder<Types, TypeList<Tail...>>::LeafTypesList
    >::Result                                                                   LeafTypesList;

    typedef typename MergeLists<
            typename Head::template NonLeafTypesList<Types>,
            typename NodeTypeListBuilder<Types, TypeList<Tail...>>::NonLeafTypesList
    >::Result                                                                   NonLeafTypesList;
};


template <typename Types>
struct NodeTypeListBuilder<Types, TypeList<>> {
    typedef TypeList<>                                                          AllTypesList;
    typedef TypeList<>                                                          LeafTypesList;
    typedef TypeList<>                                                          NonLeafTypesList;
};





template <typename Types, typename NodeTypes> struct DefaultNodeTypeListBuilder;

template <typename Types, typename Head, typename... Tail>
struct DefaultNodeTypeListBuilder<Types, TypeList<Head, Tail...>> {

    typedef typename MergeLists<
            typename Head::template Type<Types>,
            typename DefaultNodeTypeListBuilder<Types, TypeList<Tail...>>::List
    >::Result                                                                   List;
};


template <typename Types>
struct DefaultNodeTypeListBuilder<Types, TypeList<>> {
    typedef TypeList<>                                                          List;
};




template <
    typename Types1
>
class BTreeDispatchers: public Types1 {

    typedef BTreeDispatchers<Types1>                                           MyType;

    typedef typename Types1::NodeTypes                                          Types;
    typedef typename Types1::NodeList                                           NodeList_;
    typedef typename Types1::DefaultNodeList                                    DefaultNodeList_;
    typedef typename Types1::NodeBase                                           NodeBase_;

public:

    struct NodeTypesBase: Types {
        typedef NodeBase_ NodeBase;
    };

    struct AllTypes: NodeTypesBase {
        typedef typename NodeTypeListBuilder<Types, NodeList_>::AllTypesList        List;
    };

    struct LeafTypes: NodeTypesBase {
        typedef typename NodeTypeListBuilder<Types, NodeList_>::LeafTypesList       List;
    };

    struct NonLeafTypes: NodeTypesBase {
        typedef typename NodeTypeListBuilder<Types, NodeList_>::NonLeafTypesList    List;
    };

    struct DefaultTypes: NodeTypesBase {
        typedef typename DefaultNodeTypeListBuilder<Types, DefaultNodeList_>::List  List;
    };

    struct TreeTypes: NodeTypesBase {
        typedef typename NodeTypeListBuilder<Types, NodeList_>::NonLeafTypesList    List;
        typedef typename NodeTypeListBuilder<Types, NodeList_>::AllTypesList        ChildList;
    };

    typedef NDT<AllTypes>                                   NodeDispatcher;

    typedef NDT<LeafTypes>                                  LeafDispatcher;
    typedef NDT<NonLeafTypes>                               NonLeafDispatcher;
    typedef NDT<DefaultTypes>                               DefaultDispatcher;
    typedef NDTTree<TreeTypes>                              TreeDispatcher;
};



}
}

#endif
