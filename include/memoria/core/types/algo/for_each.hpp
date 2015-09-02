
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_ALGO_FOR_EACH_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_ALGO_FOR_EACH_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/typelist.hpp>

namespace memoria    {

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
	static void process(Fn&& fn, Args&&... args){
		fn.template process<Idx>(std::forward<Args>(args)...);
		ForEach<Idx + 1, Size>::process(std::forward<Fn>(fn), std::forward<Args>(args)...);
	}
};


template <Int Idx>
struct ForEach<Idx, Idx> {
	template <typename... Args>
	static void process(Args&&... args){}
};




}

#endif  //_MEMORIA_CORE_TOOLS_TYPES_ALGO_FOR_EACH_HPP
