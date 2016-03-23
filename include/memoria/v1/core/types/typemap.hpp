
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/typelist.hpp>

namespace memoria {
namespace v1 {

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
struct Type2TypeMap<T, TypeList<>, DefaultType> {
    typedef DefaultType Result;
};

template <
    typename T,
    typename ... Tail,
    typename DefaultType
>
struct Type2TypeMap<typename T::First, TypeList<T, Tail...>, DefaultType> {
    typedef typename T::Second Result;
};

template <
    typename T,
    typename Head,
    typename ... Tail,
    typename DefaultType
>
struct Type2TypeMap<T, v1::TypeList<Head, Tail...>, DefaultType> {
    typedef typename Type2TypeMap<T, TypeList<Tail...>, DefaultType>::Result Result;
};

}}