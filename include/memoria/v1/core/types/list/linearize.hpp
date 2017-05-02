
// Copyright 2014 Victor Smirnov
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

#include <memoria/v1/core/types/list/misc.hpp>
#include <memoria/v1/core/types/algo/select.hpp>

namespace memoria {
namespace v1 {

template <typename List, int32_t Depth = 0>
struct ListDepth {
    static const int32_t Value = Depth;
};


template <typename Head, typename... Tail, int32_t Depth>
struct ListDepth<TypeList<Head, Tail...>, Depth> {
    static const int32_t Value = ListDepth<TypeList<Tail...>, Depth>::Value;
};

template <typename... Head, typename... Tail, int32_t Depth>
struct ListDepth<TypeList<TypeList<Head...>, Tail...>, Depth> {
    static const int32_t Value = Max<
            int32_t,
            ListDepth<TypeList<Head...>, Depth + 1>::Value,
            ListDepth<TypeList<Tail...>, Depth>::Value
    >::Value;
};


template <int32_t Depth>
struct ListDepth<TypeList<>, Depth> {
    static const int32_t Value = Depth + 1;
};



template <typename List> struct IsPlainList {
    static const bool Value = false;
};

template <typename... List>
struct IsPlainList<TypeList<List...>> {
    static const bool Value = ListDepth<TypeList<List...>>::Value == 1;
};




namespace {

    template <typename T, int32_t MaxDepth = 1>
    struct LinearizeT {
        using Type = TypeList<T>;
    };

    // This case is not defined
    template <typename T>
    struct LinearizeT<T, 0>;


    template <typename T, typename... Tail, int32_t MaxDepth>
    struct LinearizeT<TypeList<T, Tail...>, MaxDepth>: HasType<
        IfThenElse<
                ListDepth<T>::Value < MaxDepth,
                MergeLists<
                    TypeList<T>,
                    typename LinearizeT<TypeList<Tail...>, MaxDepth>::Type
                >,
                MergeLists<
                    typename LinearizeT<T, MaxDepth>::Type,
                    typename LinearizeT<TypeList<Tail...>, MaxDepth>::Type
                >
        >
    > {};


    template <int32_t MaxDepth>
    struct LinearizeT<TypeList<>, MaxDepth> {
        using Type = TypeList<>;
    };


}

template <typename T, int32_t MaxDepth = 1>
using Linearize = typename LinearizeT<T, MaxDepth>::Type;


template <typename List, typename Set> struct ListSubsetH;

template <typename List, typename Set>
using ListSubset = typename ListSubsetH<List, Set>::Type;

template <
    typename List,
    int32_t Head,
    int32_t... Tail
>
struct ListSubsetH<List, IntList<Head, Tail...>> {
    using Type = AppendItemToList<
            Select<Head, List>,
            typename ListSubsetH<List,IntList<Tail...>>::Type
    >;
};

template <
    typename List
>
struct ListSubsetH<List, IntList<>> {
    using Type = TypeList<>;
};

}}