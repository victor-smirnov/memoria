
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/config.hpp>
#include <memoria/v1/core/types/fn_traits.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <type_traits>
#include <utility>
#include <tuple>

namespace memoria {
namespace v1 {
namespace bt {

template <typename Types, int idx> class NDT2;
template <typename Types> class NDT2<Types, -1>;

template <typename Types, int Idx> class NDTTree;
template <typename Types> class NDTTree<Types, -1>;

template <typename Types, int Idx = ListSize<typename Types::List> - 1>
class NDTTree {

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = SelectByIndex<Idx, typename Types::List>;

    static const uint64_t HASH  = Head::PAGE_HASH;
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
            return NDT2<Types, ListSize<typename Types::ChildList> - 1>::dispatchTreeConst(
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

    static const int32_t Idx = 0;

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = SelectByIndex<Idx, typename Types::List>;

    static const uint64_t HASH  = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT3 = NDTTree<Types, Idx - 1>;

public:
    using NDT2Start = NDT2<Types, ListSize<typename Types::ChildList> - 1>;



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
            MMA1_THROW(Exception()) << WhatCInfo("Can't dispatch btree node type");
        }
    }
};



template <typename Types, int Idx>
class NDT2 {

    using NodeBaseG  = typename Types::NodeBaseG;
    using Head       = SelectByIndex<Idx, typename Types::ChildList>;

    using NextNDT2 = NDT2<Types, Idx - 1>;

    static const uint64_t HASH = Head::PAGE_HASH;



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

    static const int32_t Idx = 0;

    using NodeBaseG     = typename Types::NodeBaseG;
    using Head          = SelectByIndex<Idx, typename Types::ChildList>;

    static const uint64_t HASH = Head::PAGE_HASH;



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
            MMA1_THROW(DispatchException()) << WhatInfo("Can't dispatch btree node type");
        }
    }
};




}
}}
