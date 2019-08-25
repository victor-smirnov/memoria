
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/fn_traits.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/types/list/typelist.hpp>

#include <type_traits>
#include <utility>
#include <tuple>

namespace memoria {
namespace v1 {
namespace bt {

template <typename CtrT, typename Types, int idx> class NDT2;
template <typename CtrT, typename Types> class NDT2<CtrT, Types, -1>;

template <typename CtrT, typename Types, int Idx> class NDTTree;
template <typename CtrT, typename Types> class NDTTree<CtrT, Types, -1>;

template <typename CtrT, typename Types, int Idx = ListSize<typename Types::List> - 1>
class NDTTree {

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = Select<Idx, typename Types::List>;

    static const uint64_t HASH  = Head::BLOCK_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT3 = NDTTree<CtrT, Types, Idx - 1>;

    using NodeSO        = typename Head::template SparseObject<CtrT>;
    using ConstNodeSO   = typename Head::template ConstSparseObject<CtrT>;

    CtrT& ctr_;

public:
    NDTTree(CtrT& ctr): ctr_(ctr) {}

    template <typename Functor, typename... Args>
    auto
    dispatchTree(
            const NodeBaseG& parent,
            const NodeBaseG& child,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == parent->block_type_hash())
        {
            ConstNodeSO parent_so(&ctr_, static_cast<const Head*>(parent.block()));

            return NDT2<CtrT, Types, ListSize<typename Types::ChildList> - 1>(ctr_).dispatchTreeConst(
                    parent_so,
                    child,
                    std::forward<Functor>(functor),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT3(ctr_).dispatchTree(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }
};


template <typename CtrT, typename Types>
class NDTTree<CtrT, Types, 0> {

    static const int32_t Idx = 0;

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = Select<Idx, typename Types::List>;

    static const uint64_t HASH  = Head::BLOCK_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT3 = NDTTree<CtrT, Types, Idx - 1>;

    using NodeSO        = typename Head::template SparseObject<CtrT>;
    using ConstNodeSO   = typename Head::template ConstSparseObject<CtrT>;

    CtrT& ctr_;

public:
    using NDT2Start = NDT2<CtrT, Types, ListSize<typename Types::ChildList> - 1>;

    NDTTree(CtrT& ctr): ctr_(ctr) {}


    template <typename Functor, typename... Args>
    auto
    dispatchTree(
            const NodeBaseG& parent,
            const NodeBaseG& child,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == parent->block_type_hash())
        {
            ConstNodeSO parent_so(&ctr_, static_cast<const Head*>(parent.block()));

            return NDT2Start(ctr_).dispatchTree(
                    parent_so,
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



template <typename CtrT, typename Types, int Idx>
class NDT2 {

    using NodeBaseG  = typename Types::NodeBaseG;
    using Head       = Select<Idx, typename Types::ChildList>;

    using NextNDT2 = NDT2<CtrT, Types, Idx - 1>;

    static const uint64_t HASH = Head::BLOCK_HASH;

    using NodeSO        = typename Head::template SparseObject<CtrT>;
    using ConstNodeSO   = typename Head::template ConstSparseObject<CtrT>;

    CtrT& ctr_;

public:
    NDT2(CtrT& ctr): ctr_(ctr) {}

    template <typename Node, typename Functor, typename... Args>
    auto dispatchTree(
            Node&& parent,
            const NodeBaseG& child,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == child->block_type_hash())
        {
            ConstNodeSO child_so(&ctr_, static_cast<const Head*>(child.block()));

            return functor.treeNode(
                    std::forward<Node>(parent),
                    child_so,
                    std::forward<Args>(args)...
            );
        }
        else {
            NextNDT2 dd(ctr_);
            return dd.dispatchTree(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }
};



template <typename CtrT, typename Types>
class NDT2<CtrT, Types, 0> {

    static const int32_t Idx = 0;

    using NodeBaseG     = typename Types::NodeBaseG;
    using Head          = Select<Idx, typename Types::ChildList>;

    static const uint64_t HASH = Head::BLOCK_HASH;

    using NodeSO        = typename Head::template SparseObject<CtrT>;
    using ConstNodeSO   = typename Head::template ConstSparseObject<CtrT>;

    CtrT& ctr_;

public:
    NDT2(CtrT& ctr): ctr_(ctr) {}

    template <typename Node, typename Functor, typename... Args>
    auto dispatchTree(
            Node&& parent,
            const NodeBaseG& child,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == child->block_type_hash())
        {
            ConstNodeSO child_so(&ctr_, static_cast<const Head*>(child.block()));
            return functor.treeNode(
                    std::forward<Node>(parent),
                    child_so,
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
