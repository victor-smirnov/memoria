
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_HPP

namespace memoria    	{
namespace balanced_tree {

template <typename Types, int idx> class NDT0;
template <typename Types, int idx> class NDT1;
template <typename Types, int idx> class NDT2;




template <typename Types>
class NDT: public NDT0<Types, ListSize<typename Types::List>::Value - 1> {};

template <typename Types>
class NDT0<Types, -1> {

    typedef typename Types::NodeBase NodeBase;

public:
    template <typename Functor, typename... Args>
    static void dispatch(NodeBase*, Functor &&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <template <typename> class Wrapper, typename Functor, typename... Args>
    static void wrappedDispatch(NodeBase*, Functor &&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }


    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchRtn(NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchRtn(NodeBase*, NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }


    template <typename Functor, typename... Args >
    static void dispatchConst(const NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchConstRtn(const NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor, typename... Args>
        static typename Functor::ReturnType dispatchConstRtn2(const NodeBase*, const NodeBase*, Functor&&, Args...) {
        	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }


    template <typename Functor, typename... Args>
    static void dispatch2(NodeBase* node1, NodeBase* node2, Functor&& functor, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor, typename... Args>
    static void doubleDispatch(NodeBase* node1, NodeBase* node2, Functor&& functor, Args... args) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }


    template <typename Functor, typename... Args>
    static void dispatchConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor, typename... Args>
    static void doubleDispatchConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor, typename... Args>
    static void dispatchTreeConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }


    template <typename Functor, typename... Args>
    static typename Functor::ReturnType doubleDispatchConstRtn(const NodeBase*, const NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchTreeConstRtn(const NodeBase*, const NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <
    	template <typename, bool, bool> class TreeNode,
    	typename Functor,
    	typename... Args
    >
    static typename Functor::ReturnType dispatchStaticRtn(bool, bool, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }


    template <
        	typename Functor,
        	typename... Args
    >
    static typename Functor::ReturnType dispatchStatic2Rtn(bool, bool, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <
    	template <typename, bool, bool> class TreeNode,
    	typename Functor,
    	typename... Args
    >
    static void dispatchStatic(bool, bool, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    static void buildMetadataList(MetadataList &list) {}
};



template <typename Types>
class NDT1<Types, -1> {
    typedef typename Types::NodeBase NodeBase;
public:

    template <typename Node, typename Functor, typename... Args>
    static void dispatch(Node*, NodeBase *, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Node, typename Functor, typename... Args>
    static void dispatchConst(const Node*, const NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Node, typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchConstRtn(const Node*, const NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }
};



template <typename Types>
class NDT2<Types, -1> {
    typedef typename Types::NodeBase NodeBase;
public:

    template <typename Node, typename Functor, typename... Args>
    static void dispatch(Node*, NodeBase *, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Node, typename Functor, typename... Args>
    static void dispatchTreeConst(const Node*, const NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }

    template <typename Node, typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchTreeConstRtn(const Node*, const NodeBase*, Functor&&, Args...) {
    	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
    }
};





template <typename Types, int Idx>
class NDT1 {

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::List>::Result Head;

    static const Int HASH = Head::PAGE_HASH;

public:

    template <typename Node, typename Functor, typename... Args>
    static void dispatch(Node *node1, NodeBase *node2, Functor &functor, Args... args)
    {
    	if (HASH == node2->page_type_hash())
    	{
    		functor.treeNode(
    				node1,
    				static_cast<Head*>(node2),
    				args...
    		);
    	}
    	else {
    		NDT1<Types, Idx - 1>::dispatch(node1, node2, functor, args...);
    	}
    }


    template <typename Node, typename Functor, typename... Args>
    static void dispatchConst(const Node* node1, const NodeBase* node2, Functor&& functor, Args... args)
    {
    	if (HASH == node2->page_type_hash())
    	{
    		functor.treeNode(
    				node1,
    				static_cast<const Head*>(node2),
    				args...
    		);
    	}
    	else {
    		NDT1<Types, Idx - 1>::dispatchConst(node1, node2, std::move(functor), args...);
    	}
    }


    template <typename Node, typename Functor, typename... Args>
    static void dispatchTreeConst(const Node* parent, const NodeBase* child, Functor&& functor, Args... args)
    {
    	if (HASH == child->page_type_hash())
    	{
    		functor.treeNode(
    				parent,
    				static_cast<const Head*>(child),
    				args...
    		);
    	}
    	else {
    		NDT1<Types, Idx - 1>::dispatchConst(parent, child, std::move(functor), args...);
    	}
    }


    template <typename Node, typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchConstRtn(
    		const Node* node1,
    		const NodeBase* node2,
    		Functor&& functor,
    		Args... args
    )
    {
    	if (HASH == node2->page_type_hash())
    	{
    		return functor.treeNode(
    				node1,
    				static_cast<const Head*>(node2),
    				args...
    		);
    	}
    	else {
    		return NDT1<Types, Idx - 1>::dispatchConstRtn(node1, node2, std::move(functor), args...);
    	}
    }


    template <typename Node, typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchTreeConstRtn(
    		const Node* parent,
    		const NodeBase* child,
    		Functor&& functor,
    		Args... args
    )
    {
    	if (HASH == child->page_type_hash())
    	{
    		return functor.treeNode(
    				parent,
    				static_cast<const Head*>(child),
    				args...
    		);
    	}
    	else {
    		return NDT1<Types, Idx - 1>::dispatchTreeConstRtn(parent, child, std::move(functor), args...);
    	}
    }
};



template <typename Types, int Idx>
class NDT2 {

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::ChildList>::Result Head;

    static const Int HASH = Head::PAGE_HASH;

public:

    template <typename Node, typename Functor, typename... Args>
    static void dispatchTreeConst(const Node* parent, const NodeBase* child, Functor&& functor, Args... args)
    {
    	if (HASH == child->page_type_hash())
    	{
    		functor.treeNode(
    				parent,
    				static_cast<const Head*>(child),
    				args...
    		);
    	}
    	else {
    		NDT2<Types, Idx - 1>::dispatchConst(parent, child, std::move(functor), args...);
    	}
    }


    template <typename Node, typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchTreeConstRtn(
    		const Node* parent,
    		const NodeBase* child,
    		Functor&& functor,
    		Args... args
    )
    {
    	if (HASH == child->page_type_hash())
    	{
    		return functor.treeNode(
    				parent,
    				static_cast<const Head*>(child),
    				args...
    		);
    	}
    	else {
    		return NDT2<Types, Idx - 1>::dispatchTreeConstRtn(parent, child, std::move(functor), args...);
    	}
    }
};






template <
	template <typename, bool, bool> class TreeNode,
	typename TreeNodeAdaptor
>
struct IsTreeNode {
	static const bool Value = false;
};


template <
	template <typename, bool, bool> class TreeNode,
	typename Types,
	bool root,
	bool leaf
>
struct IsTreeNode<TreeNode, NodePageAdaptor<TreeNode, Types, root, leaf>> {
	static const bool Value = true;
};



template <typename Types, int Idx>
class NDT0 {

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::List>::Result Head;

    static const Int HASH 		= Head::PAGE_HASH;
    static const bool Root 		= Head::Root;
    static const bool Leaf 		= Head::Leaf;



public:

    template <typename Functor, typename... Args>
    static void dispatch(NodeBase* node, Functor&& functor, Args... args)
    {
    	if (HASH == node->page_type_hash())
    	{
    		functor.treeNode(static_cast<Head*>(node), args...);
    	}
    	else {
    		NDT0<Types, Idx - 1>::dispatch(node, functor, args...);
    	}
    }

    template <template <typename> class Wrapper, typename Functor, typename... Args>
    static void wrappedDispatch(NodeBase* node1, NodeBase* node2, Functor&& functor, Args... args)
    {
    	if (HASH == node1->page_type_hash())
    	{
    		Wrapper<Head> wrapper(functor);
    		wrapper.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), args...);
    	}
    	else {
    		NDT0<
    			Types, Idx - 1
    		>
    		::template wrappedDispatch<Wrapper>(node1, node2, std::move(functor), args...);
    	}
    }

    template <typename Functor, typename... Args>
    static void dispatch2(NodeBase* node1, NodeBase* node2, Functor&& functor, Args... args)
    {
    	if (HASH == node1->page_type_hash())
    	{
    		functor.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), args...);
    	}
    	else {
    		NDT0<Types, Idx - 1>::dispatch2(node1, node2, std::move(functor), args...);
    	}
    }


    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchRtn(NodeBase* node, Functor&& functor, Args... args)
    {
    	if (HASH == node->page_type_hash())
    	{
    		return functor.treeNode(static_cast<Head*>(node), args...);
    	}
    	else {
    		return NDT0<Types, Idx - 1>::dispatchRtn(node, std::move(functor), args...);
    	}
    }

    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchRtn(NodeBase* node1, NodeBase* node2, Functor&& functor, Args... args)
    {
    	if (HASH == node1->page_type_hash())
    	{
    		return functor.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), args...);
    	}
    	else {
    		return NDT0<Types, Idx - 1>::dispatchRtn(node1, node2, std::move(functor), args...);
    	}
    }

    template <typename Functor, typename... Args>
    static void dispatchConst(const NodeBase* node, Functor&& functor, Args... args)
    {
    	if (HASH == node->page_type_hash())
        {
    		const Head* head = static_cast<const Head*>(node);
            functor.treeNode(head, args...);
        }
        else {
            NDT0<Types, Idx - 1>::dispatchConst(node, functor, args...);
        }
    }

    template <typename Functor, typename... Args>
    static void dispatchConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args... args)
    {
    	if (HASH == node1->page_type_hash())
    	{
    		functor.treeNode(static_cast<const Head*>(node1), static_cast<const Head*>(node2), args...);
    	}
    	else {
    		NDT0<Types, Idx - 1>::dispatchConst(node1, node2, functor, args...);
    	}
    }

    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchConstRtn(const NodeBase* node, Functor&& functor, Args... args)
    {
    	if (HASH == node->page_type_hash())
    	{
    		return functor.treeNode(static_cast<const Head*>(node), args...);
    	}
    	else {
    		return NDT0<Types, Idx - 1>::dispatchConstRtn(node, std::move(functor), args...);
    	}
    }

    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchConstRtn(const NodeBase* node, Functor& functor, Args... args)
    {
    	if (HASH == node->page_type_hash())
    	{
    		return functor.treeNode(static_cast<const Head*>(node), args...);
    	}
    	else {
    		return NDT0<Types, Idx - 1>::dispatchConstRtn(node, std::move(functor), args...);
    	}
    }


    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchConstRtn2(
    		const NodeBase* node1,
    		const NodeBase* node2,
    		Functor&& functor,
    		Args... args
    )
    {
    	if (HASH == node1->page_type_hash())
    	{
    		return functor.treeNode(static_cast<const Head*>(node1), static_cast<const Head*>(node2), args...);
    	}
    	else {
    		return NDT0<Types, Idx - 1>::dispatchConstRtn2(node1, node2, std::move(functor), args...);
    	}
    }

    template <typename Functor, typename... Args>
    static void doubleDispatch(NodeBase* node1, NodeBase* node2, Functor&& functor, Args... args)
    {
    	if (HASH == node1->page_type_hash())
    	{
    		NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatch(
    				static_cast<Head*>(node1),
    				node2,
    				functor,
    				args...
    		);
    	}
    	else {
    		NDT0<Types, Idx - 1>::doubleDispatch(node1, node2, std::move(functor), args...);
    	}
    }

    template <typename Functor, typename... Args>
    static void doubleDispatchConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args... args)
    {
    	if (HASH == node1->page_type_hash())
    	{
    		NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatchConst(
    				static_cast<const Head*>(node1),
    				node2,
    				functor,
    				args...
    		);
    	}
    	else {
    		NDT0<Types, Idx - 1>::doubleDispatchConst(node1, node2, std::move(functor), args...);
    	}
    }


    template <typename Functor, typename... Args>
    static void dispatchTreeConst(const NodeBase* parent, const NodeBase* child, Functor&& functor, Args... args)
    {
    	if (HASH == parent->page_type_hash())
    	{
    		NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>::dispatchTreeConst(
    				static_cast<const Head*>(parent),
    				child,
    				functor,
    				args...
    		);
    	}
    	else {
    		NDT0<Types, Idx - 1>::dispatchTreeConst(parent, child, std::move(functor), args...);
    	}
    }


    template <typename Functor, typename... Args>
    static typename Functor::ReturnType doubleDispatchConstRtn(
    		const NodeBase* node1,
    		const NodeBase* node2,
    		Functor&& functor,
    		Args... args
    )
    {
    	if (HASH == node1->page_type_hash())
    	{
    		return NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatchConstRtn(
    				static_cast<const Head*>(node1),
    				node2,
    				std::move(functor),
    				args...
    		);
    	}
    	else {
    		return NDT0<Types, Idx - 1>::doubleDispatchConstRtn(node1, node2, std::move(functor), args...);
    	}
    }


    template <typename Functor, typename... Args>
    static typename Functor::ReturnType dispatchTreeConstRtn(
    		const NodeBase* parent,
    		const NodeBase* child,
    		Functor&& functor,
    		Args... args
    )
    {
    	if (HASH == parent->page_type_hash())
    	{
    		return NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>::dispatchTreeConstRtn(
    				static_cast<const Head*>(parent),
    				child,
    				std::move(functor),
    				args...
    		);
    	}
    	else {
    		return NDT0<Types, Idx - 1>::dispatchTreeConstRtn(parent, child, std::move(functor), args...);
    	}
    }


    template <
    	template <typename, bool, bool> class TreeNode,
    	typename Functor,
    	typename... Args
    >
    static void dispatchStatic(bool root, bool leaf, Functor&& fn, Args... args)
    {
    	bool types_equal = IsTreeNode<TreeNode, Head>::Value;

    	if (types_equal && root == Root && leaf == Leaf)
    	{
    		const Head* head = nullptr;
    		fn.treeNode(head, args...);
    	}
    	else {
    		NDT0<
    			Types, Idx - 1
    		>
    		::template dispatchStatic<TreeNode>(root, leaf, std::move(fn), args...);
    	}
    }


    template <
    	template <typename, bool, bool> class TreeNode,
    	typename Functor,
    	typename... Args
    >
    static typename Functor::ReturnType dispatchStaticRtn(bool root, bool leaf, Functor&& fn, Args... args)
    {
    	bool types_equal = IsTreeNode<TreeNode, Head>::Value;

    	if (types_equal && root == Root && leaf == Leaf)
    	{
    		const Head* head = nullptr;
    		return fn.treeNode(head, args...);
    	}
    	else {
    		return NDT0<
    			Types, Idx - 1
    		>
    		::template dispatchStaticRtn<TreeNode>(root, leaf, std::move(fn), args...);
    	}
    }

    template <
    typename Functor,
    typename... Args
    >
    static typename Functor::ReturnType dispatchStatic2Rtn(bool root, bool leaf, Functor&& fn, Args... args)
    {
    	if (root == Root && leaf == Leaf)
    	{
    		const Head* head = nullptr;
    		return fn.treeNode(head, args...);
    	}
    	else {
    		return NDT0<
    				Types, Idx - 1
    				>
    		::template dispatchStatic2Rtn(root, leaf, std::move(fn), args...);
    	}
    }


    static void buildMetadataList(MetadataList &list) {
        Head::initMetadata();
        list.push_back(Head::page_metadata());
        NDT0<Types, Idx - 1>::buildMetadataList(list);
    }
};






}
}


#endif
