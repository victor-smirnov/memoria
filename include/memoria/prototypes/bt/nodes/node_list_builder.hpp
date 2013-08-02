
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_LIST_BUILDER_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_NODE_LIST_BUILDER_HPP

#include <memoria/core/types/typelist.hpp>


namespace memoria    	{
namespace bt	{

template <
	template <typename, bool, bool> class TreeNode,
	typename Types,
	bool root, bool leaf
>
class NodePageAdaptor;


template<
	template <typename, bool, bool> class Type
>
struct AllNodeTypes {
	template <typename Types>
	using AllTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>,
			NodePageAdaptor<Type, Types, true, false>,
			NodePageAdaptor<Type, Types, false, true>,
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using RootTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>,
			NodePageAdaptor<Type, Types, true, false>
	>;

	template <typename Types>
	using LeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>,
			NodePageAdaptor<Type, Types, false, true>
	>;

	template <typename Types>
	using NonRootTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, true>,
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using NonLeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, false>,
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using InternalTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;
};





template<
	template <typename, bool, bool> class Type
>
struct RootNodeTypes {
	template <typename Types>
	using AllTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>,
			NodePageAdaptor<Type, Types, true, false>
	>;

	template <typename Types>
	using RootTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>,
			NodePageAdaptor<Type, Types, true, false>
	>;

	template <typename Types>
	using LeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>
	>;

	template <typename Types>
	using NonRootTypesList = TypeList<>;

	template <typename Types>
	using NonLeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, false>
	>;

	template <typename Types>
	using InternalTypesList = TypeList<>;
};





template<
	template <typename, bool, bool> class Type
>
struct LeafNodeTypes {
	template <typename Types>
	using AllTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>,
			NodePageAdaptor<Type, Types, false, true>
	>;

	template <typename Types>
	using RootTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>
	>;

	template <typename Types>
	using LeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, true>,
			NodePageAdaptor<Type, Types, false, true>
	>;

	template <typename Types>
	using NonRootTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, true>
	>;

	template <typename Types>
	using NonLeafTypesList = TypeList<>;

	template <typename Types>
	using InternalTypesList = TypeList<>;
};



template<
	template <typename, bool, bool> class Type
>
struct NonRootNodeTypes {
	template <typename Types>
	using AllTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, true>,
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using RootTypesList = TypeList<>;

	template <typename Types>
	using LeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, true>
	>;

	template <typename Types>
	using NonRootTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, true>,
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using NonLeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using InternalTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;
};





template<
	template <typename, bool, bool> class Type
>
struct NonLeafNodeTypes {
	template <typename Types>
	using AllTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, false>,
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using RootTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, false>
	>;

	template <typename Types>
	using LeafTypesList = TypeList<>;

	template <typename Types>
	using NonRootTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using NonLeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, true, false>,
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using InternalTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;
};

template<
	template <typename, bool, bool> class Type
>
struct InternalNodeTypes {
	template <typename Types>
	using AllTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using RootTypesList = TypeList<>;

	template <typename Types>
	using LeafTypesList = TypeList<>;

	template <typename Types>
	using NonRootTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using NonLeafTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;

	template <typename Types>
	using InternalTypesList = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;
};





template<
	template <typename, bool, bool> class Type
>
struct RootNodeType {
	template <typename Types>
	using List = TypeList<
			NodePageAdaptor<Type, Types, true, false>
	>;
};


template<
	template <typename, bool, bool> class Type
>
struct LeafNodeType {
	template <typename Types>
	using List = TypeList<
			NodePageAdaptor<Type, Types, false, true>
	>;
};

template<
	template <typename, bool, bool> class Type
>
struct RootLeafNodeType {
	template <typename Types>
	using List = TypeList<
			NodePageAdaptor<Type, Types, true, true>
	>;
};


template<
	template <typename, bool, bool> class Type
>
struct InternalNodeType {
	template <typename Types>
	using List = TypeList<
			NodePageAdaptor<Type, Types, false, false>
	>;
};




template <typename Types, typename NodeTypes> struct NodeTypeListBuilder;


template <typename Types, typename Head, typename... Tail>
struct NodeTypeListBuilder<Types, TypeList<Head, Tail...>> {

	typedef typename MergeLists<
			typename Head::template AllTypesList<Types>,
			typename NodeTypeListBuilder<Types, TypeList<Tail...>>::AllTypesList
	>::Result 																	AllTypesList;


	typedef typename MergeLists<
			typename Head::template RootTypesList<Types>,
			typename NodeTypeListBuilder<Types, TypeList<Tail...>>::RootTypesList
	>::Result 																	RootTypesList;

	typedef typename MergeLists<
			typename Head::template LeafTypesList<Types>,
			typename NodeTypeListBuilder<Types, TypeList<Tail...>>::LeafTypesList
	>::Result 																	LeafTypesList;

	typedef typename MergeLists<
			typename Head::template NonRootTypesList<Types>,
			typename NodeTypeListBuilder<Types, TypeList<Tail...>>::NonRootTypesList
	>::Result 																	NonRootTypesList;

	typedef typename MergeLists<
			typename Head::template NonLeafTypesList<Types>,
			typename NodeTypeListBuilder<Types, TypeList<Tail...>>::NonLeafTypesList
	>::Result 																	NonLeafTypesList;

	typedef typename MergeLists<
			typename Head::template InternalTypesList<Types>,
			typename NodeTypeListBuilder<Types, TypeList<Tail...>>::InternalTypesList
	>::Result 																	InternalTypesList;
};


template <typename Types>
struct NodeTypeListBuilder<Types, TypeList<>> {
	typedef TypeList<> 															AllTypesList;
	typedef TypeList<> 															RootTypesList;
	typedef TypeList<> 															LeafTypesList;
	typedef TypeList<> 															NonRootTypesList;
	typedef TypeList<> 															NonLeafTypesList;
	typedef TypeList<> 															InternalTypesList;
};





template <typename Types, typename NodeTypes> struct DefaultNodeTypeListBuilder;

template <typename Types, typename Head, typename... Tail>
struct DefaultNodeTypeListBuilder<Types, TypeList<Head, Tail...>> {

	typedef typename MergeLists<
			typename Head::template List<Types>,
			typename DefaultNodeTypeListBuilder<Types, TypeList<Tail...>>::List
	>::Result 																	List;
};


template <typename Types>
struct DefaultNodeTypeListBuilder<Types, TypeList<>> {
	typedef TypeList<> 															List;
};




template <
    typename Types1
>
class BTreeDispatchers2: public Types1 {

    typedef BTreeDispatchers2<Types1>                                         				MyType;

    typedef typename Types1::NodeTypes														Types;
    typedef typename Types1::NodeList														NodeList_;
    typedef typename Types1::DefaultNodeList												DefaultNodeList_;
    typedef typename Types1::NodeBase 														NodeBase_;

public:

    struct NodeTypesBase: Types {
        typedef NodeBase_ NodeBase;
    };

    struct AllTypes: NodeTypesBase {
        typedef typename NodeTypeListBuilder<Types, NodeList_>::AllTypesList 		List;
    };

    struct RootTypes: NodeTypesBase {
    	typedef typename NodeTypeListBuilder<Types, NodeList_>::RootTypesList 		List;
    };

    struct LeafTypes: NodeTypesBase {
    	typedef typename NodeTypeListBuilder<Types, NodeList_>::LeafTypesList 		List;
    };

    struct NonLeafTypes: NodeTypesBase {
    	typedef typename NodeTypeListBuilder<Types, NodeList_>::NonLeafTypesList 	List;
    };

    struct NonRootTypes: NodeTypesBase {
    	typedef typename NodeTypeListBuilder<Types, NodeList_>::NonRootTypesList 	List;
    };

    struct InternalTypes: NodeTypesBase {
    	typedef typename NodeTypeListBuilder<Types, NodeList_>::InternalTypesList 	List;
    };

    struct DefaultTypes: NodeTypesBase {
    	typedef typename DefaultNodeTypeListBuilder<Types, DefaultNodeList_>::List 	List;
    };

    struct TreeTypes: NodeTypesBase {
    	typedef typename NodeTypeListBuilder<Types, NodeList_>::NonLeafTypesList 	List;
    	typedef typename NodeTypeListBuilder<Types, NodeList_>::AllTypesList 		ChildList;
    };

    typedef NDT<AllTypes>                                   NodeDispatcher;
    typedef NDT<RootTypes>                                  RootDispatcher;
    typedef NDT<LeafTypes>                                  LeafDispatcher;
    typedef NDT<NonLeafTypes>                               NonLeafDispatcher;
    typedef NDT<NonRootTypes>                               NonRootDispatcher;
    typedef NDT<InternalTypes>                              InternalDispatcher;
    typedef NDT<DefaultTypes>                               DefaultDispatcher;
    typedef NDT<TreeTypes>                               	TreeDispatcher;
};



}
}

#endif