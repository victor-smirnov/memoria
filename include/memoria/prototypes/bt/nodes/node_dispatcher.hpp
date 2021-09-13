
// Copyright 2011-2021 Victor Smirnov
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

#include <memoria/profiles/common/block.hpp>

#include <memoria/prototypes/bt/nodes/node_dispatcher_tree.hpp>

#include <vector>

namespace memoria {
namespace bt {

template <
    template <typename> class TreeNode,
    typename Types
>
class NodePageAdaptor;

template <typename CtrT, typename Types, int idx> class NDT0;
template <typename CtrT, typename Types> class NDT0<CtrT, Types, -1>;

template <typename CtrT, typename Types>
struct NDT: public NDT0<CtrT, Types, ListSize<typename Types::List> - 1> {
    NDT() noexcept: NDT0<CtrT, Types, ListSize<typename Types::List> - 1>() {}
    NDT(CtrT* ctr) noexcept: NDT0<CtrT, Types, ListSize<typename Types::List> - 1>(ctr)
    {}
};


template <
    template <typename> class TreeNode,
    typename TreeNodeAdaptor
>
struct IsTreeNode {
    static constexpr bool Value = false;
};


template <
    template <typename> class TreeNode,
    typename Types
>
struct IsTreeNode<TreeNode, NodePageAdaptor<TreeNode, Types>> {
    static constexpr bool Value = true;
};



template <typename CtrT, typename Types, int Idx>
class NDT0 {

    using MyType = NDT0<CtrT, Types, Idx>;
public:
    using Head = Select<Idx, typename Types::List>;

private:
    static constexpr uint64_t HASH  = Head::BLOCK_HASH;
    static constexpr bool Leaf      = Head::Leaf;

    using NodeSO        = typename Head::template SparseObject<CtrT>;
    using ConstNodeSO   = NodeSO;

    using NextNDT0 = NDT0<CtrT, Types, Idx - 1>;

    CtrT* ctr_;

public:
    using TreeNodePtr = typename Types::TreeNodePtr;
    using TreeNodeConstPtr = typename Types::TreeNodeConstPtr;

public:
    NDT0() noexcept {}
    NDT0(CtrT* ctr) noexcept: ctr_(ctr) {}

    void set_ctr(CtrT* ctr) noexcept {
        ctr_ = ctr;
    }

    template <typename Functor, typename... Args>
    auto dispatch(const TreeNodePtr& node, Functor&& functor, Args&&... args) const noexcept
    {
        if (HASH == node->block_type_hash())
        {
            NodeSO node_so(ctr_, static_cast<Head*>(node.block()));
            return functor.treeNode(node_so, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch(node, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    auto dispatch(const TreeNodePtr& node1, const TreeNodePtr& node2, Functor&& functor, Args&&... args) const noexcept
    {
        if (HASH == node1->block_type_hash())
        {
            NodeSO node1_so(ctr_, static_cast<Head*>(node1.block()));
            NodeSO node2_so(ctr_, static_cast<Head*>(node2.block()));

            return functor.treeNode(node1_so, node2_so, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    auto dispatch_1st_const(const TreeNodeConstPtr& node1, const TreeNodePtr& node2, Functor&& functor, Args&&... args) const noexcept
    {
        if (HASH == node1->block_type_hash())
        {
            ConstNodeSO node1_so(ctr_, const_cast<Head*>(static_cast<const Head*>(node1.block())));
            NodeSO node2_so(ctr_, static_cast<Head*>(node2.block()));

            return functor.treeNode(node1_so, node2_so, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch_1st_const(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }

    template <typename Functor, typename... Args>
    auto dispatch(const TreeNodeConstPtr& node, Functor&& functor, Args&&... args) const noexcept
    {
        if (HASH == node->block_type_hash())
        {
            const ConstNodeSO node_so(ctr_, const_cast<Head*>(static_cast<const Head*>(node.block())));
            return functor.treeNode(node_so, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch(node, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Functor, typename... Args>
    auto dispatch(
            const TreeNodeConstPtr& node1,
            const TreeNodeConstPtr& node2,
            Functor&& functor,
            Args&&... args
    ) const noexcept
    {
        if (HASH == node1->block_type_hash())
        {
            const ConstNodeSO node1_so(ctr_, const_cast<Head*>(static_cast<const Head*>(node1.block())));
            const ConstNodeSO node2_so(ctr_, const_cast<Head*>(static_cast<const Head*>(node2.block())));
            return functor.treeNode(node1_so, node2_so, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).dispatch(node1, node2, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }




    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    auto dispatch(bool leaf, Functor&& fn, Args&&... args) const noexcept
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            const ConstNodeSO node_so(ctr_, static_cast<Head*>(nullptr));
            return fn.treeNode(node_so, std::forward<Args>(args)...);
        }
        else {
            return NextNDT0(ctr_).template dispatch<TreeNode>(leaf, std::forward<Functor>(fn), std::forward<Args>(args)...);
        }
    }

    template <
        typename Functor,
        typename... Args
    >
    auto dispatch2(bool leaf, Functor&& fn, Args&&... args) const noexcept
    {
        if (leaf == Leaf)
        {
            const ConstNodeSO node_so(ctr_, static_cast<Head*>(nullptr));
            return fn.treeNode(node_so, std::forward<Args>(args)...);
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
    using Head      = Select<Idx, typename Types::List>;

private:
    static const uint64_t HASH  = Head::BLOCK_HASH;
    static const bool Leaf      = Head::Leaf;

    using NodeSO        = typename Head::template SparseObject<CtrT>;
    using ConstNodeSO   = NodeSO;

    CtrT* ctr_;

public:
    using TreeNodePtr = typename Types::TreeNodePtr;
    using TreeNodeConstPtr = typename Types::TreeNodeConstPtr;

public:
    NDT0() noexcept {}
    NDT0(CtrT* ctr) noexcept: ctr_(ctr) {}

    void set_ctr(CtrT* ctr) noexcept {
        ctr_ = ctr;
    }

    template <typename Functor, typename... Args>
    auto dispatch(const TreeNodePtr& node, Functor&& functor, Args&&... args) const noexcept ->
        typename detail::ResultOfFn<decltype(functor.treeNode(std::declval<NodeSO&>(), std::forward<Args>(args)...))>::Type
    {
        if (HASH == node->block_type_hash())
        {
            NodeSO node_so(ctr_, static_cast<Head*>(node.block()));
            return functor.treeNode(node_so, std::forward<Args>(args)...);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch btree node type: {}", node->block_type_hash());
        }
    }

    template <typename Functor, typename... Args>
    auto dispatch(const TreeNodePtr& node1, const TreeNodePtr& node2, Functor&& functor, Args&&... args) const noexcept ->
        typename detail::ResultOfFn<decltype(functor.treeNode(std::declval<NodeSO>(), std::declval<NodeSO>(), std::forward<Args>(args)...))>::Type
    {
        if (HASH == node1->block_type_hash())
        {
            NodeSO node1_so(ctr_, static_cast<Head*>(node1.block()));
            NodeSO node2_so(ctr_, static_cast<Head*>(node2.block()));
            return functor.treeNode(node1_so, node2_so, std::forward<Args>(args)...);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch btree node type: {}", node1->block_type_hash());
        }
    }

    template <typename Functor, typename... Args>
    auto dispatch_1st_const(const TreeNodeConstPtr& node1, const TreeNodePtr& node2, Functor&& functor, Args&&... args) const noexcept ->
        typename detail::ResultOfFn<decltype(functor.treeNode(std::declval<ConstNodeSO>(), std::declval<NodeSO>(), std::forward<Args>(args)...))>::Type
    {
        if (HASH == node1->block_type_hash())
        {
            ConstNodeSO node1_so(ctr_, const_cast<Head*>(static_cast<const Head*>(node1.block())));
            NodeSO node2_so(ctr_, static_cast<Head*>(node2.block()));
            return functor.treeNode(node1_so, node2_so, std::forward<Args>(args)...);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch btree node type: {}", node1->block_type_hash());
        }

    }


    template <typename Functor, typename... Args>
    auto dispatch(const TreeNodeConstPtr& node, Functor&& functor, Args&&... args) const noexcept ->
        typename detail::ResultOfFn<decltype(functor.treeNode(std::declval<ConstNodeSO>(), std::forward<Args>(args)...))>::Type
    {
        if (HASH == node->block_type_hash())
        {
            const ConstNodeSO node_so(ctr_, const_cast<Head*>(static_cast<const Head*>(node.block())));
            return functor.treeNode(node_so, std::forward<Args>(args)...);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch btree node type: {}", node->block_type_hash());
        }
    }


    template <typename Functor, typename... Args>
    auto dispatch(
            const TreeNodeConstPtr& node1,
            const TreeNodeConstPtr& node2,
            Functor&& functor,
            Args&&... args
    ) const noexcept ->
        typename detail::ResultOfFn<decltype(functor.treeNode(std::declval<ConstNodeSO>(), std::declval<ConstNodeSO>(), std::forward<Args>(args)...))>::Type
    {
        if (HASH == node1->block_type_hash())
        {
            const ConstNodeSO node1_so(ctr_, const_cast<Head*>(static_cast<const Head*>(node1.block())));
            const ConstNodeSO node2_so(ctr_, const_cast<Head*>(static_cast<const Head*>(node2.block())));
            return functor.treeNode(node1_so, node2_so, std::forward<Args>(args)...);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch btree node type: {}", node1->block_type_hash());
        }
    }

    template <
        template <typename> class TreeNode,
        typename Functor,
        typename... Args
    >
    auto dispatch(bool leaf, Functor&& fn, Args&&... args) const ->
        typename detail::ResultOfFn<decltype(fn.treeNode(std::declval<ConstNodeSO>(), std::forward<Args>(args)...))>::Type
    {
        bool types_equal = IsTreeNode<TreeNode, Head>::Value;

        if (types_equal && leaf == Leaf)
        {
            ConstNodeSO node_so(ctr_, static_cast<Head*>(nullptr));
            return fn.treeNode(node_so, std::forward<Args>(args)...);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch btree node type");
        }
    }

    template <
        typename Functor,
        typename... Args
    >
    auto dispatch2(bool leaf, Functor&& fn, Args&&... args) const ->
        typename detail::ResultOfFn<decltype(fn.treeNode(std::declval<ConstNodeSO>(), std::declval<NodeSO>(), std::forward<Args>(args)...))>::Type
    {
        if (leaf == Leaf)
        {
            const ConstNodeSO node_so(ctr_, static_cast<Head*>(nullptr));
            return fn.treeNode(node_so, std::forward<Args>(args)...);
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch btree node type");
        }
    }

    template <typename T>
    static void build_metadata_list(std::vector<T> &list)
    {
        list.push_back(Head::block_operations());
    }
};

}}
