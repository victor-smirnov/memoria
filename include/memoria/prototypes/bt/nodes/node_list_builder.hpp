
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
    template <typename BrachTypes, typename LeafTypes>
    using AllTypesList = TypeList<
            NodePageAdaptor<Type, LeafTypes>
    >;

    template <typename LeafTypes>
    using LeafTypesList = TypeList<
            NodePageAdaptor<Type, LeafTypes>
    >;

    template <typename Types>
    using BranchTypesList = TypeList<>;
};





template<
    template <typename> class Type
>
struct BranchNodeTypes {
    template <typename BranchTypes, typename LeafTypes>
    using AllTypesList = TypeList<
            NodePageAdaptor<Type, BranchTypes>
    >;

    template <typename LeafTypes>
    using LeafTypesList = TypeList<>;

    template <typename BranchTypes>
    using BranchTypesList = TypeList<
            NodePageAdaptor<Type, BranchTypes>
    >;
};



template<
    template <typename> class NodeType
>
struct TreeNodeType {
    template <typename Types>
    using Type = NodePageAdaptor<NodeType, Types>;
};


template <typename BranchTypes, typename LeafTypes, typename NodeTypes> struct NodeTypeListBuilder;


template <typename BranchTypes, typename LeafTypes, typename Head, typename... Tail>
struct NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<Head, Tail...>> {

    typedef typename MergeLists<
            typename Head::template AllTypesList<BranchTypes, LeafTypes>,
            typename NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<Tail...>>::AllTypesList
    >::Result                                                                   AllTypesList;


    typedef typename MergeLists<
            typename Head::template LeafTypesList<LeafTypes>,
            typename NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<Tail...>>::LeafTypesList
    >::Result                                                                   LeafTypesList;

    typedef typename MergeLists<
            typename Head::template BranchTypesList<BranchTypes>,
            typename NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<Tail...>>::BranchTypesList
    >::Result                                                                   BranchTypesList;
};


template <typename BranchTypes, typename LeafTypes>
struct NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<>> {
    typedef TypeList<>                                                          AllTypesList;
    typedef TypeList<>                                                          LeafTypesList;
    typedef TypeList<>                                                          BranchTypesList;
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
    typename Types
>
class BTreeDispatchers {

	using MyType 			= BTreeDispatchers<Types>;

	using BranchTypes 		= typename Types::BranchNodeTypes;
    using LeafTypes 		= typename Types::LeafNodeTypes;

    using NodeTypesList 	= typename Types::NodeTypesList;
    using NodeBaseG_ 		= typename Types::NodeBaseG;

    using DefaultBranchNodeTypesList 	= typename Types::DefaultBranchNodeTypesList;
    using DefaultLeafNodeTypesList 		= typename Types::DefaultLeafNodeTypesList;

public:
    struct NodeTypesBase {
        using NodeBaseG = NodeBaseG_;
    };

    struct AllDTypes: NodeTypesBase {
        using List = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::AllTypesList;
    };

    struct LeafDTypes: NodeTypesBase {
    	using List = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::LeafTypesList;
    };

    struct BranchDTypes: NodeTypesBase {
    	using List = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::BranchTypesList;
    };

    struct DefaultDTypes: NodeTypesBase {
    	using List = typename MergeLists<
    			typename DefaultNodeTypeListBuilder<LeafTypes, DefaultLeafNodeTypesList>::List,
    			typename DefaultNodeTypeListBuilder<BranchTypes, DefaultBranchNodeTypesList>::List
    	>::Result;
    };

    struct TreeDTypes: NodeTypesBase {
        using List 		= typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::BranchTypesList;
        using ChildList = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::AllTypesList;
    };

    using NodeDispatcher 		= NDT<AllDTypes>;
    using LeafDispatcher 		= NDT<LeafDTypes>;
    using NonLeafDispatcher 	= NDT<BranchDTypes>;
    using DefaultDispatcher 	= NDT<DefaultDTypes>;
    using TreeDispatcher 		= NDTTree<TreeDTypes>;
};

}
}

#endif
