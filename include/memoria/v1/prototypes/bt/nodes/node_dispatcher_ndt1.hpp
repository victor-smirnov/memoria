
// Copyright Victor Smirnov 2011-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/fn_traits.hpp>

#include <type_traits>

#include <utility>
#include <tuple>

namespace memoria {
namespace v1 {
namespace bt {

template <typename Types, int idx> class NDT1;
template <typename Types> class NDT1<Types, -1>;

template <typename Types, int Idx>
class NDT1 {

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = SelectByIndex<Idx, typename Types::List>;

    using NextNDT1 = NDT1<Types, Idx - 1>;

    static const Int HASH = Head::PAGE_HASH;

public:

    template <typename Node, typename Functor, typename... Args>
    static auto dispatch(
            Node* node1,
            NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    )
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
    using Head      = SelectByIndex<Idx, typename Types::List>;

    static const Int HASH = Head::PAGE_HASH;


public:

    template <typename Node, typename Functor, typename... Args>
    static auto
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
    static auto
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
}}