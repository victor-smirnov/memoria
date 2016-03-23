
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/typelist.hpp>



namespace memoria {


template <
        typename Config,
        typename List,
        template <typename Config_, typename Item, typename LastResult> class Handler,
        typename Accumulator
>
struct ForEachItem;


template <
        typename Config,
        template <typename, typename, typename> class Handler,
        typename Accumulator
>
struct ForEachItem<Config, TypeList<>, Handler, Accumulator> {
    typedef Accumulator                                                         Result;
};

template <
        typename Config,
        typename Head,
        typename ... Tail,
        template <typename, typename, typename> class Handler,
        typename Accumulator
>
struct ForEachItem<Config, TypeList<Head, Tail...>, Handler, Accumulator> {
    typedef typename ForEachItem<
                Config,
                TypeList<Tail...>,
                Handler,
                Handler<Config, Head, Accumulator>
    >::Result                                                                   Result;
};



template <Int Idx, Int Size>
struct ForEach {
    template <typename Fn, typename... Args>
    static void process(Fn&& fn, Args&&... args)
    {
        if (fn.template process<Idx>(std::forward<Args>(args)...))
        {
            ForEach<Idx + 1, Size>::process(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }
};


template <Int Idx>
struct ForEach<Idx, Idx> {
    template <typename... Args>
    static void process(Args&&... args){}
};


template <Int V1, Int V2> struct IfLess
{
    template <typename Fn, typename ElseFn, typename... Args>
    static auto process(Fn&& fn, ElseFn&& else_fn, Args&&... args) {
        return fn.template process<V1>(std::forward<Args>(args)...);
    }
};

template <Int V2>
struct IfLess<V2, V2>
{
    template <typename Fn, typename ElseFn, typename... Args>
    static auto process(Fn&& fn, ElseFn&& else_fn, Args&&... args) {
        return else_fn.template process<V2>(std::forward<Args>(args)...);
    }
};


template <typename List, template <typename Item> class Mapper> struct MapTL;

template <typename Head, typename... Tail, template <typename Item> class Mapper>
struct MapTL<TL<Head, Tail...>, Mapper> {
    using Type = MergeLists<
            typename Mapper<Head>::Type,
            typename MapTL<TL<Tail...>, Mapper>::Type
    >;
};

template <template <typename Item> class Mapper>
struct MapTL<TL<>, Mapper>{
    using Type = TL<>;
};

template <typename T>
struct WithType {
    using Type = T;
};

}
