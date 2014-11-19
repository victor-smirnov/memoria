
// Copyright Victor Smirnov 2011-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_NDT2_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_DISPATCHERS_DISPATCHER_NDT2_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/fn_traits.hpp>

#include <type_traits>
#include <utility>
#include <tuple>

namespace memoria   {
namespace bt 		{

template <typename Types, int idx> class NDT2;
template <typename Types> class NDT2<Types, -1>;

template <typename Types, int Idx> class NDTTree;
template <typename Types> class NDTTree<Types, -1>;

template <typename Types, int Idx = ListSize<typename Types::List>::Value - 1>
class NDTTree {

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::List>::Result Head;

    static const Int HASH       = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT3 = NDTTree<Types, Idx - 1>;

public:


    template <typename Functor, typename... Args>
    static void dispatchTreeConst(const NodeBase* parent, const NodeBase* child, Functor&& functor, Args&&... args)
    {
        if (HASH == parent->page_type_hash())
        {
            NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>::dispatchTreeConst(
                    static_cast<const Head*>(parent),
                    child,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
        	NextNDT3::dispatchTreeConst(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }



    template <typename... Args>
    using DispatchTreeConstFnType = auto(Args...) -> decltype(NextNDT3::template dispatchTreeConstRtn(std::declval<Args>()...));

    template <typename Fn, typename... Args>
    using DispatchTreeConstRtnType = typename FnTraits<DispatchTreeConstFnType<typename std::remove_reference<Fn>::type, Args...>>::RtnType;


    template <typename Functor, typename... Args>
    static DispatchTreeConstRtnType<const NodeBase*, const NodeBase*, Functor, Args...>
    dispatchTreeConstRtn(
            const NodeBase* parent,
            const NodeBase* child,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == parent->page_type_hash())
        {
            return NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>::dispatchTreeConstRtn(
                    static_cast<const Head*>(parent),
                    child,
                    std::forward<Functor>(functor),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT3::dispatchTreeConstRtn(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }
};


template <typename Types>
class NDTTree<Types, 0> {

	static const Int Idx = 0;

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::List>::Result Head;

    static const Int HASH       = Head::PAGE_HASH;
    static const bool Leaf      = Head::Leaf;

    using NextNDT3 = NDTTree<Types, Idx - 1>;

public:


    template <typename Functor, typename... Args>
    static void dispatchTreeConst(const NodeBase* parent, const NodeBase* child, Functor&& functor, Args&&... args)
    {
        if (HASH == parent->page_type_hash())
        {
            NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>::dispatchTreeConst(
                    static_cast<const Head*>(parent),
                    child,
                    functor,
                    std::forward<Args>(args)...
            );
        }
        else {
        	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }

    using NDT2Start = NDT2<Types, ListSize<typename Types::ChildList>::Value - 1>;

    template <typename... Args>
    using DispatchTreeConstFnType = auto(Args...) -> decltype(NDT2Start::template dispatchTreeConstRtn(std::declval<Args>()...));

    template <typename Fn, typename... Args>
    using DispatchTreeConstRtnType = typename FnTraits<DispatchTreeConstFnType<typename std::remove_reference<Fn>::type, Args...>>::RtnType;


    template <typename Functor, typename... Args>
    static DispatchTreeConstRtnType<const NodeBase*, const NodeBase*, Functor, Args...>
    dispatchTreeConstRtn(
            const NodeBase* parent,
            const NodeBase* child,
            Functor&& functor,
            Args&&... args
    )
    {
        if (HASH == parent->page_type_hash())
        {
            return NDT2Start::dispatchTreeConstRtn(
                    static_cast<const Head*>(parent),
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

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::ChildList>::Result Head;

    using NextNDT2 = NDT2<Types, Idx - 1>;

    static const Int HASH = Head::PAGE_HASH;

    template <typename T, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template treeNode(std::declval<Args>()...));

    template <typename Fn, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, T...>>::RtnType;


public:

    template <typename Node, typename Functor, typename... Args>
    static void dispatchTreeConst(const Node* parent, const NodeBase* child, Functor&& functor, Args&&... args)
    {
        if (HASH == child->page_type_hash())
        {
            functor.treeNode(
                    parent,
                    static_cast<const Head*>(child),
                    std::forward<Args>(args)...
            );
        }
        else {
            NextNDT2::dispatchConst(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }


    template <typename Node, typename Functor, typename... Args>
    static auto dispatchTreeConstRtn(
            const Node* parent,
            const NodeBase* child,
            Functor&& functor,
            Args&&... args
    )
    -> RtnType<Functor, const Node*, const Head*, Args...>
    {
        if (HASH == child->page_type_hash())
        {
            return functor.treeNode(
                    parent,
                    static_cast<const Head*>(child),
                    std::forward<Args>(args)...
            );
        }
        else {
            return NextNDT2::dispatchTreeConstRtn(parent, child, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }
    }
};



template <typename Types>
class NDT2<Types, 0> {

	static const Int Idx = 0;

    typedef typename Types::NodeBase NodeBase;
    typedef typename SelectByIndexTool<Idx, typename Types::ChildList>::Result Head;

    static const Int HASH = Head::PAGE_HASH;

    template <typename T, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template treeNode(std::declval<Args>()...));

    template <typename Fn, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, T...>>::RtnType;


public:

    template <typename Node, typename Functor, typename... Args>
    static void dispatchTreeConst(const Node* parent, const NodeBase* child, Functor&& functor, Args&&... args)
    {
        if (HASH == child->page_type_hash())
        {
            functor.treeNode(
                    parent,
                    static_cast<const Head*>(child),
                    std::forward<Args>(args)...
            );
        }
        else {
        	throw DispatchException(MEMORIA_SOURCE, "Can't dispatch btree node type");
        }
    }




    template <typename Node, typename Functor, typename... Args>
    static auto dispatchTreeConstRtn(
            const Node* parent,
            const NodeBase* child,
            Functor&& functor,
            Args&&... args
    )
    -> RtnType<Functor, const Node*, const Head*, Args...>
    {
        if (HASH == child->page_type_hash())
        {
            return functor.treeNode(
                    parent,
                    static_cast<const Head*>(child),
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
