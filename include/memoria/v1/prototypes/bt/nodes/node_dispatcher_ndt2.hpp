
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
namespace bt        {

template <typename Types, int idx> class NDT2;
template <typename Types> class NDT2<Types, -1>;

template <typename Types, int Idx> class NDTTree;
template <typename Types> class NDTTree<Types, -1>;

template <typename Types, int Idx = ListSize<typename Types::List>::Value - 1>
class NDTTree {

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = SelectByIndex<Idx, typename Types::List>;

    static const Int HASH       = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT3 = NDTTree<Types, Idx - 1>;

public:


    template <typename Functor, typename... Args>
    static auto
    dispatchTree(
            const NodeBaseG& parent,
            const NodeBaseG& child,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == parent->page_type_hash())
        {
            return NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>::dispatchTreeConst(
                    static_cast<const Head*>(parent.page()),
                    child,
                    std::forward<Functor>(functor),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT3::dispatchTree(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }
};


template <typename Types>
class NDTTree<Types, 0> {

    static const Int Idx = 0;

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = SelectByIndex<Idx, typename Types::List>;

    static const Int HASH       = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT3 = NDTTree<Types, Idx - 1>;

public:
    using NDT2Start = NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>;



    template <typename Functor, typename... Args>
    static auto
    dispatchTree(
            const NodeBaseG& parent,
            const NodeBaseG& child,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == parent->page_type_hash())
        {
            return NDT2Start::dispatchTree(
                    static_cast<const Head*>(parent.page()),
                    child,
                    std::forward<Functor>(functor),
                    std::forward<Args>(args)...
            );
        }
        else {
            throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }
};



template <typename Types, int Idx>
class NDT2 {

    using NodeBaseG  = typename Types::NodeBaseG;
    using Head       = SelectByIndex<Idx, typename Types::ChildList>;

    using NextNDT2 = NDT2<Types, Idx - 1>;

    static const Int HASH = Head::PAGE_HASH;



public:
    template <typename Node, typename Functor, typename... Args>
    static auto dispatchTree(
            const Node* parent,
            const NodeBaseG& child,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == child->page_type_hash())
        {
            return functor.treeNode(
                    parent,
                    static_cast<const Head*>(child.page()),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT2::dispatchTree(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }
};



template <typename Types>
class NDT2<Types, 0> {

    static const Int Idx = 0;

    using NodeBaseG     = typename Types::NodeBaseG;
    using Head          = SelectByIndex<Idx, typename Types::ChildList>;

    static const Int HASH = Head::PAGE_HASH;



public:
    template <typename Node, typename Functor, typename... Args>
    static auto dispatchTree(
            const Node* parent,
            const NodeBaseG& child,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == child->page_type_hash())
        {
            return functor.treeNode(
                    parent,
                    static_cast<const Head*>(child.page()),
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