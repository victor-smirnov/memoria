
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

#include <memoria/v1/profiles/common/block.hpp>

#include <memoria/v1/prototypes/bt/nodes/node_dispatcher_ndt1.hpp>
#include <memoria/v1/prototypes/bt/nodes/node_dispatcher_ndt2.hpp>

#include <vector>

namespace memoria {
namespace v1      {
namespace bt      {


template <typename CtrT, typename Types, int idx> class NDT0;
template <typename CtrT, typename Types> class NDT0<CtrT, Types, -1>;

template <typename CtrT, typename Types>
struct NDT: public NDT0<CtrT, Types, ListSize<typename Types::List> - 1> {
    NDT(CtrT& ctr): NDT0<CtrT, Types, ListSize<typename Types::List> - 1>(ctr)
    {}
};


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



template <typename CtrT, typename Types, int Idx>
class NDT0 {

    using MyType = NDT0<CtrT, Types, Idx>;
public:
    using Head = SelectByIndex<Idx, typename Types::List>;

private:
    static const uint64_t HASH  = Head::BLOCK_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT0 = NDT0<CtrT, Types, Idx - 1>;

    CtrT& ctr_;

public:
    using NodeBaseG = typename Types::NodeBaseG;

public:
    NDT0(CtrT& ctr): ctr_(ctr) {}

    template <template <typename> class Wrapper, typename Functor, typename... Args>
    auto
    wrappedDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args) const
    {
        if (HASH == node1->block_type_hash())
        {
            Wrapper<Head> wrapper(functor);
            return wrapper.treeNode(static_cast<Head*>(node1.block()), static_cast<Head*>(node2.block()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).template wrappedDispatch<Wrapper>(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    auto dispatch(NodeBaseG& node, Functor&& functor, Args&&... args) const
    {
        if (HASH == node->block_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node.block()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch(node, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    auto dispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args) const
    {
        if (HASH == node1->block_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node1.block()), static_cast<Head*>(node2.block()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    auto dispatch(const NodeBaseG& node, Functor&& functor, Args&&... args) const
    {
        if (HASH == node->block_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node.block()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch(node, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    auto dispatch(
            const NodeBaseG& node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == node1->block_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node1.block()), static_cast<const Head*>(node2.block()), std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    auto
    doubleDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args) const
    {
        if (HASH == node1->block_type_hash())
        {
            return NDT1<CtrT, Types, ListSize<typename Types::List> - 1>::dispatch(
                    static_cast<Head*>(node1.block()),
                    node2,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT0(ctr_).doubleDispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }



    template <typename Functor, typename... Args>
    auto
    doubleDispatch(
            const NodeBaseG& node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == node1->block_type_hash())
        {
            return NDT1<CtrT, Types, ListSize<typename Types::List> - 1>::dispatchConstRtn(
                    static_cast<const Head*>(node1.block()),
                    node2,
                    std::forward<Functor>(functor),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT0(ctr_).doubleDispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }



    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    auto dispatch(bool leaf, Functor&& fn, Args&&... args) const
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).template dispatch<TreeNode>(leaf, std::forward<Functor>(fn), std::forward<Args>(args)...);
        }
    }

    template <
        typename Functor,
        typename... Args
    >
    auto dispatch2(bool leaf, Functor&& fn, Args&&... args) const
    {
        if (leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).template dispatch2(leaf, std::forward<Functor>(fn), std::forward<Args>(args)...);
        }
    }

    template <typename T>
    static void build_metadata_list(std::vector<T> &list)
    {
        list.push_back(Head::block_operations());
        NextNDT0::build_metadata_list(list);
    }
};






template <typename CtrT, typename Types>
class NDT0<CtrT, Types, 0> {

    static const int32_t Idx = 0;
public:
    using Head      = SelectByIndex<Idx, typename Types::List>;

private:
    static const uint64_t HASH  = Head::BLOCK_HASH;
    static const bool Leaf      = Head::Leaf;

    CtrT& ctr_;

public:
    using NodeBaseG = typename Types::NodeBaseG;

    using StartNDT1 = NDT1<CtrT, Types, ListSize<typename Types::List> - 1>;


public:
    NDT0(CtrT& ctr): ctr_(ctr) {}


    template <template <typename> class Wrapper, typename Functor, typename... Args>
    auto
    wrappedDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args) const
    {
        if (HASH == node1->block_type_hash())
        {
            Wrapper<Head> wrapper(functor);
            return wrapper.treeNode(static_cast<Head*>(node1.block()), static_cast<Head*>(node2.block()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    auto dispatch(NodeBaseG& node, Functor&& functor, Args&&... args) const
    {
        if (HASH == node->block_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node.block()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatInfo(fmt::format8(u"Can't dispatch btree node type: {}", node->block_type_hash()));
        }
    }

    template <typename Functor, typename... Args>
    auto dispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args) const
    {
        if (HASH == node1->block_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node1.block()), static_cast<Head*>(node2.block()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    auto dispatch(const NodeBaseG& node, Functor&& functor, Args&&... args) const
    {
        if (HASH == node->block_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node.block()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    auto dispatch(
            const NodeBaseG& node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    ) const
    {
        if (HASH == node1->block_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node1.block()), static_cast<const Head*>(node2.block()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    auto
    doubleDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args) const
    {
        if (HASH == node1->block_type_hash())
        {
            return NDT1<CtrT, Types, ListSize<typename Types::List> - 1>::dispatch(
                    static_cast<Head*>(node1.block()),
                    node2,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    auto
    doubleDispatch(const NodeBaseG& node1, const NodeBaseG& node2, Functor&& functor, Args&&... args) const
    {
        if (HASH == node1->block_type_hash())
        {
            return NDT1<CtrT, Types, ListSize<typename Types::List> - 1>::dispatch(
                    static_cast<const Head*>(node1.block()),
                    node2,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }

    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    auto dispatch(bool leaf, Functor&& fn, Args&&... args) const
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }

    template <
        typename Functor,
        typename... Args
    >
    auto dispatch2(bool leaf, Functor&& fn, Args&&... args) const
    {
        if (leaf == Leaf)
        {
            const Head* head = nullptr;
            return fn.treeNode(head, std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }

    template <typename T>
    static void build_metadata_list(std::vector<T> &list)
    {
        list.push_back(Head::block_operations());
    }
};

}
}}
