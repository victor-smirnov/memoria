
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

#include <type_traits>

#include <utility>
#include <tuple>

namespace memoria {
namespace v1 {
namespace bt {

template <typename CtrT, typename Types, int idx> class NDT1;
template <typename CtrT, typename Types> class NDT1<CtrT, Types, -1>;

template <typename CtrT, typename Types, int Idx>
class NDT1 {

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = SelectByIndex<Idx, typename Types::List>;

    using NextNDT1 = NDT1<CtrT, Types, Idx - 1>;

    static const uint64_t HASH = Head::BLOCK_HASH;

    CtrT& ctr_;

public:
    NDT1(CtrT& ctr): ctr_(ctr) {}

    template <typename Node, typename Functor, typename... Args>
    auto dispatch(
            Node* node1,
            NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == node2->block_type_hash())
        {
            return functor.treeNode(
                    node1,
                    static_cast<Head*>(node2.block()),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT1(ctr_).dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Node, typename Functor, typename... Args>
    auto dispatch(
            const Node* node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == node2->block_type_hash())
        {
            return functor.treeNode(
                    node1,
                    static_cast<const Head*>(node2.block()),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT1(ctr_).dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }
};



template <typename CtrT, typename Types>
class NDT1<CtrT, Types, 0> {

    static const int32_t Idx = 0;

    using NodeBaseG = typename Types::NodeBaseG;
    using Head      = SelectByIndex<Idx, typename Types::List>;

    static const uint64_t HASH = Head::BLOCK_HASH;

    CtrT& ctr_;

public:
    NDT1(CtrT& ctr): ctr_(ctr) {}

    template <typename Node, typename Functor, typename... Args>
    auto
    dispatch(Node *node1, NodeBaseG& node2, Functor&& functor, Args&& ... args) const
    {
        if (HASH == node2->block_type_hash())
        {
            return functor.treeNode(
                    node1,
                    static_cast<Head*>(node2.block()),
                    std::forward<Args>(args)...
            );
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }


    template <typename Node, typename Functor, typename... Args>
    auto
    dispatch(const Node* node1, const NodeBaseG& node2, Functor&& functor, Args&&... args) const
    {
        if (HASH == node2->block_type_hash())
        {
            return functor.treeNode(
                    node1,
                    static_cast<const Head*>(node2.block()),
                    std::forward<Args>(args)...
            );
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }
};


}
}}
