
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_NODE_TYPEMAP_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_NODE_TYPEMAP_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/typelist.hpp>

namespace memoria    {

template <
    typename T,
    typename TChain,
    typename DefaultType = TypeNotFound<T>
>
struct Type2TypeMap;

template <
    typename T,
    typename DefaultType
>
struct Type2TypeMap<T, VTL<>, DefaultType> {
    typedef DefaultType Result;
};

template <
    typename T,
    typename ... Tail,
    typename DefaultType
>
struct Type2TypeMap<typename T::First, VTL<T, Tail...>, DefaultType> {
    typedef typename T::Second Result;
};

template <
    typename T,
    typename Head,
    typename ... Tail,
    typename DefaultType
>
struct Type2TypeMap<T, memoria::VTL<Head, Tail...>, DefaultType> {
    typedef typename Type2TypeMap<T, VTL<Tail...>, DefaultType>::Result Result;
};

}
#endif  //_MEMORIA_CORE_TOOLS_TYPES_NODE_TYPEMAP_HPP


