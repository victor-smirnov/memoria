
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
struct ForEach;
/*
template <
        typename Config,
        template <typename, typename, typename> class Handler,
        typename Accumulator
>
struct ForEach<Config, NullType, Handler, Accumulator> {
    typedef Accumulator                                                         Result;
};

template <
        typename Config,
        typename Head,
        typename Tail,
        template <typename, typename, typename> class Handler,
        typename Accumulator
>
struct ForEach<Config, TL<Head, Tail>, Handler, Accumulator> {
    typedef typename ForEach<
                Config,
                Tail,
                Handler,
                Handler<Config, Head, Accumulator>
    >::Result                                                                   Result;
};


*/




template <
        typename Config,
        template <typename, typename, typename> class Handler,
        typename Accumulator
>
struct ForEach<Config, VTL<>, Handler, Accumulator> {
    typedef Accumulator                                                         Result;
};

template <
        typename Config,
        typename Head,
        typename ... Tail,
        template <typename, typename, typename> class Handler,
        typename Accumulator
>
struct ForEach<Config, VTL<Head, Tail...>, Handler, Accumulator> {
    typedef typename ForEach<
                Config,
                VTL<Tail...>,
                Handler,
                Handler<Config, Head, Accumulator>
    >::Result                                                                   Result;
};



}

#endif  //_MEMORIA_CORE_TOOLS_TYPES_ALGO_FOR_EACH_HPP
