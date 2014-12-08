
// Copyright Victor Smirnov 2011-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_NDT1_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_NDT1_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/fn_traits.hpp>

#include <type_traits>

#include <utility>
#include <tuple>

namespace memoria       {
namespace bt {

template <typename Types, int idx> class NDT1;
template <typename Types> class NDT1<Types, -1>;

template <typename Types, int Idx>
class NDT1 {

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = typename SelectByIndexTool<Idx, typename Types::List>::Type;

    using NextNDT1 = NDT1<Types, Idx - 1>;

    static const Int HASH = Head::PAGE_HASH;

    template <typename T, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template treeNode(std::declval<Args>()...));

    template <typename Fn, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, T...>>::RtnType;


public:

    template <typename Node, typename Functor, typename... Args>
    static auto dispatch(
            Node* node1,
            NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    )
    -> RtnType<Functor, Node*, Head*, Args...>
    {
        if (HASH == node2->page_type_hash())
        {
            return functor.treeNode(
                    node1,
                    static_cast<Head*>(node2.page()),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT1::dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Node, typename Functor, typename... Args>
    static auto dispatch(
            const Node* node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    )
    -> RtnType<Functor, const Node*, const Head*, Args...>
    {
        if (HASH == node2->page_type_hash())
        {
            return functor.treeNode(
                    node1,
                    static_cast<const Head*>(node2.page()),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT1::dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }
};



template <typename Types>
class NDT1<Types, 0> {

    static const Int Idx = 0;

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = typename SelectByIndexTool<Idx, typename Types::List>::Type;

    static const Int HASH = Head::PAGE_HASH;

    template <typename T, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template treeNode(std::declval<Args>()...));

    template <typename Fn, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, T...>>::RtnType;


public:

    template <typename Node, typename Functor, typename... Args>
    static RtnType<Functor, Node*, Head*, Args...>
    dispatch(Node *node1, NodeBaseG& node2, Functor&& functor, Args&& ... args)
    {
        if (HASH == node2->page_type_hash())
        {
            return functor.treeNode(
                    node1,
                    static_cast<Head*>(node2.page()),
                    std::forward<Args>(args)...
            );
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }


    template <typename Node, typename Functor, typename... Args>
    static RtnType<Functor, const Node*, const Head*, Args...>
    dispatch(const Node* node1, const NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node2->page_type_hash())
        {
            return functor.treeNode(
                    node1,
                    static_cast<const Head*>(node2.page()),
                    std::forward<Args>(args)...
            );
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }
};


}
}


#endif
