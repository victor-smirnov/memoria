
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

#include <memoria/v1/prototypes/bt/nodes/node_dispatcher_ndt1.hpp>
#include <memoria/v1/prototypes/bt/nodes/node_dispatcher_ndt2.hpp>


namespace memoria {
namespace v1 {
namespace bt            {


template <typename Types, int idx> class NDT0;
template <typename Types> class NDT0<Types, -1>;

template <typename Types>
class NDT: public NDT0<Types, ListSize<typename Types::List> - 1> {};


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
public:
    using Head = SelectByIndex<Idx, typename Types::List>;

private:
    static const uint64_t HASH  = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT0 = NDT0<Types, Idx - 1>;

public:
    using NodeBaseG = typename Types::NodeBaseG;

public:
    template <template <typename> class Wrapper, typename Functor, typename... Args>
    static auto
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
    static auto
    doubleDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List> - 1>::dispatch(
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
    static auto
    doubleDispatch(
            const NodeBaseG& node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List> - 1>::dispatchConstRtn(
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

    static const int32_t Idx = 0;
public:
    using Head      = SelectByIndex<Idx, typename Types::List>;

private:
    static const uint64_t HASH  = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

public:
    using NodeBaseG = typename Types::NodeBaseG;

    using StartNDT1 = NDT1<Types, ListSize<typename Types::List> - 1>;




public:


    template <template <typename> class Wrapper, typename Functor, typename... Args>
    static auto
    wrappedDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            Wrapper<Head> wrapper(functor);
            return wrapper.treeNode(static_cast<Head*>(node1.page()), static_cast<Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatch(NodeBaseG& node, Functor&& functor, Args&&... args)
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node.page()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatInfo(fmt::format8(u"Can't dispatch btree node type: {}", node->page_type_hash()));
        }
    }

    template <typename Functor, typename... Args>
    static auto dispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<Head*>(node1.page()), static_cast<Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatch(const NodeBaseG& node, Functor&& functor, Args&&... args)
    {
        if (HASH == node->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node.page()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }


    template <typename Functor, typename... Args>
    static auto dispatch(
            const NodeBaseG& node1,
            const NodeBaseG& node2,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == node1->page_type_hash())
        {
            return functor.treeNode(static_cast<const Head*>(node1.page()), static_cast<const Head*>(node2.page()), std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatCInfo("Can't dispatch btree node type");
        }
    }

    template <typename Functor, typename... Args>
    static auto
    doubleDispatch(NodeBaseG& node1, NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List> - 1>::dispatch(
                    static_cast<Head*>(node1.page()),
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
    static auto
    doubleDispatch(const NodeBaseG& node1, const NodeBaseG& node2, Functor&& functor, Args&&... args)
    {
        if (HASH == node1->page_type_hash())
        {
            return NDT1<Types, ListSize<typename Types::List> - 1>::dispatch(
                    static_cast<const Head*>(node1.page()),
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
    static auto dispatch(bool leaf, Functor&& fn, Args&&... args)
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
    static auto dispatch2(bool leaf, Functor&& fn, Args&&... args)
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


    static void buildMetadataList(MetadataList &list) {
        Head::initMetadata();
        list.push_back(Head::page_metadata());
    }
};

}
}}
