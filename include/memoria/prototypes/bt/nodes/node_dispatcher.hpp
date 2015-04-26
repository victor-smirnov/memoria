
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_HPP

#include <memoria/prototypes/bt/nodes/node_dispatcher_ndt1.hpp>
#include <memoria/prototypes/bt/nodes/node_dispatcher_ndt2.hpp>

namespace memoria       {
namespace bt            {


template <typename Dispatcher, typename Fn, typename... Args>
using DispatchRtnType = decltype(Dispatcher::dispatch(std::declval<typename Dispatcher::NodeBaseG&>(), std::declval<Fn>(), std::declval<Args>()...));

template <typename Dispatcher, typename Fn, typename... Args>
using DispatchConstRtnType = decltype(Dispatcher::dispatch(std::declval<const typename Dispatcher::NodeBaseG&>(), std::declval<Fn>(), std::declval<Args>()...));


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

	using MyType = NDT0<Types, Idx>;

    using Head      = SelectByIndex<Idx, typename Types::List>;

    static const Int HASH       = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT0 = NDT0<Types, Idx - 1>;

public:
    using NodeBaseG = typename Types::NodeBaseG;

    template <typename T, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template treeNode(std::declval<Args>()...));

    template <typename Fn, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, T...>>::RtnType;

    template <typename... Args>
    using DoubleDispatchFnType = auto(Args...) -> decltype(NextNDT0::template doubleDispatch(std::declval<Args>()...));

    template <typename Fn, typename... Args>
    using DoubleDispatchRtnType = typename FnTraits<DoubleDispatchFnType<typename std::remove_reference<Fn>::type, Args...>>::RtnType;

public:
    template <template <typename> class Wrapper, typename Functor, typename... Args>
    static RtnType<Functor, Head*, Head*, Args...>
    wrappedDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            Wrapper<Head> wrapper(functor);
            return wrapper.treeNode(static_cast<Head*>(node1.page()), static_cast<Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::template wrappedDispatch<Wrapper>(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatch(NodeBaseG& node, Functor&& functor, Args&&... args)
    -> RtnType<Functor, Head*, Args...>
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node.page()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::dispatch(node, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static auto dispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    -> RtnType<Functor, Head*, Head*, Args...>
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node1.page()), static_cast<Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static auto dispatch(const NodeBaseG& node, Functor&& functor, Args&&... args)
    -> RtnType<Functor, const Head*, Args...>
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node.page()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::dispatch(node, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatch(
            const NodeBaseG& node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    )
    -> RtnType<Functor, const Head*, const Head*, Args...>
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node1.page()), static_cast<const Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    static DoubleDispatchRtnType<NodeBaseG&, NodeBaseG&, Functor, Args...>
    doubleDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatch(
                    static_cast<Head*>(node1.page()),
                    node2,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT0::doubleDispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }



    template <typename Functor, typename... Args>
    static DoubleDispatchRtnType<NextNDT0, const NodeBaseG&, const NodeBaseG&, Functor, Args...>
    doubleDispatch(
            const NodeBaseG& node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatchConstRtn(
                    static_cast<const Head*>(node1.page()),
                    node2,
                    std::forward<Functor>(functor),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT0::doubleDispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }



    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    static auto dispatch(bool leaf, Functor&& fn, Args&&... args)
    -> RtnType<Functor, const Head*, const Head*, Args...>
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::template dispatch<TreeNode>(leaf, std::forward<Functor>(fn), std::forward<Args>(args)...);
        }
    }

    template <
        typename Functor,
        typename... Args
    >
    static auto dispatch2(bool leaf, Functor&& fn, Args&&... args)
    -> RtnType<Functor, const Head*, Args...>
    {
        if (leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0::template dispatch2(leaf, std::forward<Functor>(fn), std::forward<Args>(args)...);
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

    using Head      = SelectByIndex<Idx, typename Types::List>;

    static const Int HASH       = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

public:
    using NodeBaseG = typename Types::NodeBaseG;

    template <typename T, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template treeNode(std::declval<Args>()...));

    template <typename Fn, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, T...>>::RtnType;

    using StartNDT1 = NDT1<Types, ListSize<typename Types::List>::Value - 1>;


    template <typename... Args>
    using DoubleDispatchFnType = auto(Args...) -> decltype(
            StartNDT1::template dispatch(std::declval<Args>()...)
    );

    template <typename Fn, typename... Args>
    using DoubleDispatchRtnType = typename FnTraits<
            DoubleDispatchFnType<typename std::remove_reference<Fn>::type, Args...>
    >::RtnType;


public:


    template <template <typename> class Wrapper, typename Functor, typename... Args>
    static RtnType<Functor, Head*, Head*, Args...>
    wrappedDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            Wrapper<Head> wrapper(functor);
            return wrapper.treeNode(static_cast<Head*>(node1.page()), static_cast<Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatch(NodeBaseG& node, Functor&& functor, Args&&... args)
    -> RtnType<Functor, Head*, Args...>
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node.page()), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static auto dispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    -> RtnType<Functor, Head*, Head*, Args...>
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node1.page()), static_cast<Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatch(const NodeBaseG& node, Functor&& functor, Args&&... args)
    -> RtnType<Functor, const Head*, Args...>
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node.page()), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatch(
            const NodeBaseG& node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    )
    -> RtnType<Functor, const Head*, const Head*, Args...>
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node1.page()), static_cast<const Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static DoubleDispatchRtnType<Head*, NodeBaseG&, Functor, Args...>
    doubleDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatch(
                    static_cast<Head*>(node1.page()),
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
    static DoubleDispatchRtnType<const Head*, const NodeBaseG&, Functor, Args...>
    doubleDispatch(const NodeBaseG& node1, const NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List>::Value - 1>::dispatch(
                    static_cast<const Head*>(node1.page()),
                    node2,
                    functor,
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
    static auto dispatch(bool leaf, Functor&& fn, Args&&... args)
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
    static auto dispatch2(bool leaf, Functor&& fn, Args&&... args)
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
