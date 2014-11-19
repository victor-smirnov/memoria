
// Copyright Victor Smirnov 2011-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_HPP

#include <memoria/prototypes/bt/nodes/node_dispatcher_ndt1.hpp>
#include <memoria/prototypes/bt/nodes/node_dispatcher_ndt2.hpp>

namespace memoria       {
namespace bt            {

template <typename Types, int idx> class NDT0;
template <typename Types> class NDT0<Types, -1>;

template <typename Types>
class NDT: public NDT0<Types, ListSize<typename Types::List>::Value - 1> {};


template <
    template <typename> class TreeNode,
    typename TreeNodeAdaptor
>
struct IsTreeNode {
    static const bool Value = false;
};


template <
    template <typename> class TreeNode,
    typename Types
>
struct IsTreeNode<TreeNode, NodePageAdaptor<TreeNode, Types>> {
    static const bool Value = true;
};



template <typename Types, int Idx>
class NDT0 {

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::List>::Result Head;

    static const Int HASH       = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT0 = NDT0<Types, Idx - 1>;

    template <typename T, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template treeNode(std::declval<Args>()...));

    template <typename Fn, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, T...>>::RtnType;

public:

    template <typename Functor, typename... Args>
    static void dispatch(NodeBase* node, Functor&& functor, Args&&... args)
    {
        if (HASH == node->page_type_hash())
        {
            functor.treeNode(static_cast<Head*>(node), std::forward<Args>(args)...);
        }
        else {
            NextNDT0::dispatch(node, functor, std::forward<Args>(args)...);
        }
    }

    template <template <typename> class Wrapper, typename Functor, typename... Args>
    static void wrappedDispatch(NodeBase* node1, NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            Wrapper<Head> wrapper(functor);
            wrapper.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            NextNDT0::template wrappedDispatch<Wrapper>(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static void dispatch2(NodeBase* node1, NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            functor.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            NextNDT0::dispatch2(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatchRtn(NodeBase* node, Functor&& functor, Args&&... args)
    -> RtnType<Functor, Head*, Args...>
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::dispatchRtn(node, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static auto dispatchRtn(NodeBase* node1, NodeBase* node2, Functor&& functor, Args&&... args)
    -> RtnType<Functor, Head*, Head*, Args...>
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::dispatchRtn(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static void dispatchConst(const NodeBase* node, Functor&& functor, Args&&... args)
    {
        if (HASH == node->page_type_hash())
        {
            const Head* head = static_cast<const Head*>(node);
            functor.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            NextNDT0::dispatchConst(node, functor, std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static void dispatchConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            functor.treeNode(static_cast<const Head*>(node1), static_cast<const Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            NextNDT0::dispatchConst(node1, node2, functor, std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static auto dispatchConstRtn(const NodeBase* node, Functor&& functor, Args&&... args)
    -> RtnType<Functor, const Head*, Args...>
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::dispatchConstRtn(node, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatchConstRtn2(
            const NodeBase* node1,
            const NodeBase* node2,
            Functor&& functor,
            Args&&... args
    )
    -> RtnType<Functor, const Head*, const Head*, Args...>
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node1), static_cast<const Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::dispatchConstRtn2(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static void doubleDispatch(NodeBase* node1, NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatch(
                    static_cast<Head*>(node1),
                    node2,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            NextNDT0::doubleDispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static void doubleDispatchConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatchConst(
                    static_cast<const Head*>(node1),
                    node2,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            NextNDT0::doubleDispatchConst(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    static void dispatchTreeConst(const NodeBase* parent, const NodeBase* child, Functor&& functor, Args&&... args)
    {
        if (HASH == parent->page_type_hash())
        {
            NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>::dispatchTreeConst(
                    static_cast<const Head*>(parent),
                    child,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            NextNDT0::dispatchTreeConst(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename T, typename... Args>
    using DoubleDispatchConstFnType = auto(Args...) -> decltype(T::template doubleDispatchConstRtn(std::declval<Args>()...));

    template <typename T, typename Fn, typename... Args>
    using DoubleDispatchConstRtnType = typename FnTraits<DoubleDispatchConstFnType<T, typename std::remove_reference<Fn>::type, Args...>>::RtnType;

    template <typename Functor, typename... Args>
    static DoubleDispatchConstRtnType<NextNDT0, const NodeBase*, const NodeBase*, Functor, Args...>
    doubleDispatchConstRtn(
            const NodeBase* node1,
            const NodeBase* node2,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatchConstRtn(
                    static_cast<const Head*>(node1),
                    node2,
                    std::forward<Functor>(functor),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT0::doubleDispatchConstRtn(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    static void dispatchStatic(bool leaf, Functor&& fn, Args&&... args)
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            const Head* head = nullptr;
            fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            NextNDT0::template dispatchStatic<TreeNode>(leaf, std::forward<Functor>(fn), std::forward<Args>(args)...);
        }
    }


    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    static auto dispatchStaticRtn(bool leaf, Functor&& fn, Args&&... args)
    -> RtnType<Functor, const Head*, const Head*, Args...>
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::template dispatchStaticRtn<TreeNode>(leaf, std::forward<Functor>(fn), std::forward<Args>(args)...);
        }
    }

    template <
        typename Functor,
        typename... Args
    >
    static auto dispatchStatic2Rtn(bool leaf, Functor&& fn, Args&&... args)
    -> RtnType<Functor, const Head*, Args...>
    {
        if (leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::template dispatchStatic2Rtn(leaf, std::forward<Functor>(fn), std::forward<Args>(args)...);
        }
    }


    static void buildMetadataList(MetadataList &list) {
        Head::initMetadata();
        list.push_back(Head::page_metadata());
        NextNDT0::buildMetadataList(list);
    }
};






template <typename Types>
class NDT0<Types, 0> {

    static const Int Idx = 0;

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::List>::Result Head;

    static const Int HASH       = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    template <typename T, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template treeNode(std::declval<Args>()...));

    template <typename Fn, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, T...>>::RtnType;

public:

    template <typename Functor, typename... Args>
    static void dispatch(NodeBase* node, Functor&& functor, Args&&... args)
    {
        if (HASH == node->page_type_hash())
        {
            functor.treeNode(static_cast<Head*>(node), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <template <typename> class Wrapper, typename Functor, typename... Args>
    static void wrappedDispatch(NodeBase* node1, NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            Wrapper<Head> wrapper(functor);
            wrapper.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static void dispatch2(NodeBase* node1, NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            functor.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatchRtn(NodeBase* node, Functor&& functor, Args&&... args)
    -> RtnType<Functor, Head*, Args...>
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static auto dispatchRtn(NodeBase* node1, NodeBase* node2, Functor&& functor, Args&&... args)
    -> RtnType<Functor, Head*, Head*, Args...>
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node1), static_cast<Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static void dispatchConst(const NodeBase* node, Functor&& functor, Args&&... args)
    {
        if (HASH == node->page_type_hash())
        {
            const Head* head = static_cast<const Head*>(node);
            functor.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static void dispatchConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            functor.treeNode(static_cast<const Head*>(node1), static_cast<const Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static auto dispatchConstRtn(const NodeBase* node, Functor&& functor, Args&&... args)
    -> RtnType<Functor, const Head*, Args...>
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatchConstRtn2(
            const NodeBase* node1,
            const NodeBase* node2,
            Functor&& functor,
            Args&&... args
    )
    -> RtnType<Functor, const Head*, const Head*, Args...>
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node1), static_cast<const Head*>(node2), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static void doubleDispatch(NodeBase* node1, NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatch(
                    static_cast<Head*>(node1),
                    node2,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static void doubleDispatchConst(const NodeBase* node1, const NodeBase* node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatchConst(
                    static_cast<const Head*>(node1),
                    node2,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static void dispatchTreeConst(const NodeBase* parent, const NodeBase* child, Functor&& functor, Args&&... args)
    {
        if (HASH == parent->page_type_hash())
        {
            NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>::dispatchTreeConst(
                    static_cast<const Head*>(parent),
                    child,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    using StartNDT1 = NDT1<Types, ListSize<typename Types::List>::Value - 1>;

    template <typename... Args>
    using DoubleDispatchConstFnType = auto(Args...) -> decltype(
            StartNDT1::template dispatchConstRtn(std::declval<Args>()...)
    );

    template <typename Fn, typename... Args>
    using DoubleDispatchConstRtnType = typename FnTraits<
            DoubleDispatchConstFnType<typename std::remove_reference<Fn>::type, Args...>
    >::RtnType;


    template <typename Functor, typename... Args>
    static DoubleDispatchConstRtnType<const NodeBase*, const NodeBase*, Functor, Args...>
    doubleDispatchConstRtn(
            const NodeBase* node1,
            const NodeBase* node2,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == node1->page_type_hash())
        {
            return StartNDT1::dispatchConstRtn(
                    static_cast<const Head*>(node1),
                    node2,
                    std::forward<Functor>(functor),
                    std::forward<Args>(args)...
            );
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }




    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    static void dispatchStatic(bool leaf, Functor&& fn, Args&&... args)
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            const Head* head = nullptr;
            fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    static auto dispatchStaticRtn(bool leaf, Functor&& fn, Args&&... args)
    -> RtnType<Functor, const Head*, Args...>
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <
        typename Functor,
        typename... Args
    >
    static auto dispatchStatic2Rtn(bool leaf, Functor&& fn, Args&&... args)
    -> RtnType<Functor, const Head*, Args...>
    {
        if (leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    static void buildMetadataList(MetadataList &list) {
        Head::initMetadata();
        list.push_back(Head::page_metadata());
    }
};

}
}


#endif
