
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MISC_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MISC_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>

namespace memoria {
namespace bt1     {

template <typename MyType, typename BranchPath, typename LeafPath>
class LeveledNodeWalkerBase {

    struct LeafStreamFn {
        template <Int StreamIdx, typename... Args>
        using RtnFnType = auto(Args...) -> decltype(
                std::declval<MyType>().template leafStream<StreamIdx>(std::declval<Args>()...)
        );

        template <Int StreamIdx, typename Fn, typename... Args>
        using RtnType = typename FnTraits<RtnFnType<StreamIdx, Fn, Args...>>::RtnType;


        MyType& walker_;

        LeafStreamFn(MyType& walker): walker_(walker) {}

        template <Int StreamIdx, typename Stream, typename... Args>
        RtnType<StreamIdx, const Stream*, Args...>
        stream(const Stream* stream, Args&&... args)
        {
            return walker_.template leafStream<StreamIdx>(stream, args...);
        }
    };


    struct NonLeafStreamFn {
        template <Int StreamIdx, typename... Args>
        using RtnFnType = auto(Args...) -> decltype(
                std::declval<MyType>().template nonLeafStream<StreamIdx>(std::declval<Args>()...)
        );

        template <Int StreamIdx, typename Fn, typename... Args>
        using RtnType = typename FnTraits<RtnFnType<StreamIdx, Fn, Args...>>::RtnType;


        MyType& walker_;

        NonLeafStreamFn(MyType& walker): walker_(walker) {}

        template <Int StreamIdx, typename Stream, typename... Args>
        RtnType<StreamIdx, const Stream*, Args...>
        stream(const Stream* stream, Args&&... args)
        {
            return walker_.template nonLeafStream<StreamIdx>(stream, args...);
        }
    };

    template <typename T, typename... Args>
    using BranchRtnFnType = auto(Args...) -> decltype(
        std::declval<T>().template processStream<BranchPath>(std::declval<Args>()...)
    );

    template <typename T, typename Fn, typename... Args>
    using BranchRtnType = typename FnTraits<BranchRtnFnType<T, Fn, Args...>>::RtnType;

    template <typename T, typename... Args>
    using LeafRtnFnType = auto(Args...) -> decltype(
        std::declval<T>().template processStream<LeafPath>(std::declval<Args>()...)
    );

    template <typename T, typename Fn, typename... Args>
    using LeafRtnType = typename FnTraits<LeafRtnFnType<T, Fn, Args...>>::RtnType;

public:

    template <typename NodeTypes, typename... Args>
    LeafRtnType<const bt::LeafNode<NodeTypes>, LeafStreamFn, Args...>
    treeNode(const bt::LeafNode<NodeTypes>* node, Args&&... args)
    {
        return node->template processStream<LeafPath>(LeafStreamFn(self()), args...);
    }

    template <typename NodeTypes, typename... Args>
    BranchRtnType<const bt::BranchNode<NodeTypes>, NonLeafStreamFn, Args...>
    treeNode(const bt::BranchNode<NodeTypes>* node, Args&&... args)
    {
        return node->template processStream<BranchPath>(NonLeafStreamFn(self()), args...);
    }

    template <typename NodeTypes, typename... Args>
    LeafRtnType<bt::LeafNode<NodeTypes>, LeafStreamFn, Args...>
    treeNode(bt::LeafNode<NodeTypes>* node, Args&&... args)
    {
        return node->template processStream<LeafPath>(LeafStreamFn(self()), args...);
    }

    template <typename NodeTypes, typename... Args>
    BranchRtnType<bt::BranchNode<NodeTypes>, NonLeafStreamFn, Args...>
    treeNode(bt::BranchNode<NodeTypes>* node, Args&&... args)
    {
        return node->template processStream<BranchPath>(NonLeafStreamFn(self()), args...);
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};




template <typename MyType, typename BranchPath, typename LeafPath>
struct NodeWalkerBase {
private:

    template <typename T, typename... Args>
    using BranchRtnFnType = auto(Args...) -> decltype(
            std::declval<T>().template processStream<BranchPath>(std::declval<Args>()...)
    );

    template <typename T, typename Fn, typename... Args>
    using BranchRtnType = typename FnTraits<BranchRtnFnType<T, Fn, Args...>>::RtnType;


    template <typename T, typename... Args>
    using LeafRtnFnType = auto(Args...) -> decltype(
            std::declval<T>().template processStream<LeafPath>(std::declval<Args>()...)
    );

    template <typename T, typename Fn, typename... Args>
    using LeafRtnType = typename FnTraits<LeafRtnFnType<T, Fn, Args...>>::RtnType;

public:
    template <typename NodeTypes, typename... Args>
    LeafRtnType<bt::LeafNode<NodeTypes>, MyType, Args...>
    treeNode(bt::LeafNode<NodeTypes>* node, Args&&... args)
    {
        return node->template processStream<LeafPath>(self(), args...);
    }

    template <typename NodeTypes, typename... Args>
    BranchRtnType<bt::BranchNode<NodeTypes>, MyType, Args...>
    treeNode(bt::BranchNode<NodeTypes>* node, Args&&... args)
    {
        return node->template processStream<BranchPath>(self(), args...);
    }

    template <typename NodeTypes, typename... Args>
    LeafRtnType<const bt::LeafNode<NodeTypes>, MyType, Args...>
    treeNode(const bt::LeafNode<NodeTypes>* node, Args&&... args)
    {
        return node->template processStream<LeafPath>(self(), args...);
    }

    template <typename NodeTypes, typename... Args>
    BranchRtnType<bt::BranchNode<NodeTypes>, MyType, Args...>
    treeNode(const bt::BranchNode<NodeTypes>* node, Args&&... args)
    {
        return node->template processStream<BranchPath>(self(), args...);
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};





}
}

#endif
