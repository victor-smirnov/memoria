
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

#include <memoria/core/types/list/linearize.hpp>
#include <memoria/core/types/list/typelist.hpp>
#include <memoria/core/types/list/sublist.hpp>
#include <memoria/core/types/algo/select.hpp>

namespace memoria {
namespace list_tree {

    
namespace _ {    

    template <typename List, typename Path, int32_t Depth = 1, int32_t Ptr = 0> struct LeafCountH;

    template <
        typename Head,
        typename... Tail,
        int32_t Idx,
        int32_t... PathTail,
        int32_t Depth,
        int32_t Ptr
    >
    struct LeafCountH<TypeList<Head, Tail...>, IntList<Idx, PathTail...>, Depth, Ptr> {
        static const int32_t PrefixSize = ListSize<Linearize<TypeList<Head>, Depth>>;
        static const int32_t Value = PrefixSize + LeafCountH<
            TypeList<Tail...>,
            IntList<Idx, PathTail...>,
            Depth,
            Ptr + 1
        >::Value;
    };


    template <
        typename Head,
        typename... Tail,
        int32_t Idx,
        int32_t... PathTail,
        int32_t Depth
    >
    struct LeafCountH<TypeList<Head, Tail...>, IntList<Idx, PathTail...>, Depth, Idx> {
        static const int32_t Value = LeafCountH<
            Head,
            IntList<PathTail...>, Depth
        >::Value;
    };



    template <
        typename Head,
        typename... Tail,
        int32_t Depth,
        int32_t Idx
    >
    struct LeafCountH<TypeList<Head, Tail...>, IntList<>, Depth, Idx>: HasValue<
        int32_t,
        LeafCountH<
            TypeList<Head, Tail...>,
            IntList<0>, Depth
        >::Value
    > {};


    template <
        typename T,
        int32_t Depth,
        int32_t Idx
    >
    struct LeafCountH<T, IntList<>, Depth, Idx>: HasValue<int32_t, 0> {};
}

template <typename List, typename Path, int32_t Depth = 1, int32_t Ptr = 0> 
constexpr int32_t LeafCount = _::LeafCountH<List, Path, Depth, Ptr>::Value;


namespace _ {

    template <typename List, typename Path, int32_t Ptr = 0> struct SubtreeH;

    template <
        typename Head,
        typename... Tail,
        int32_t Idx,
        int32_t... PathTail,
        int32_t Ptr
    >
    struct SubtreeH<TypeList<Head, Tail...>, IntList<Idx, PathTail...>, Ptr>: HasType<
        typename SubtreeH<
            TypeList<Tail...>,
            IntList<Idx, PathTail...>,
            Ptr + 1
        >::Type
    > {};


    template <
        typename Head,
        typename... Tail,
        int32_t Idx,
        int32_t... PathTail
    >
    struct SubtreeH<TypeList<Head, Tail...>, IntList<Idx, PathTail...>, Idx>: HasType<
        IfThenElse<
                (sizeof...(PathTail) > 0),
                typename SubtreeH<Head, IntList<PathTail...>>::Type,
                Head
        >
    > {};

    template <
        typename Head,
        typename... Tail,
        int32_t Idx
    >
    struct SubtreeH<TypeList<Head, Tail...>, IntList<>, Idx>: HasType<TypeList<Head, Tail...>> {};


    template <
        typename T,
        int32_t Idx
    >
    struct SubtreeH<T, IntList<>, Idx>: HasType<T> {};

}

template <typename List, typename Path> 
using Subtree = typename _::SubtreeH<List, Path>::Type;


template <typename List, typename Path = IntList<>, int32_t Depth = 1>
constexpr int32_t SubtreeLeafCount = ListSize<
    Linearize<
        Subtree<List, Path>,
        Depth
    >
>;


template <typename List, typename Path, int32_t Depth = 1>
constexpr int32_t LeafCountInf = LeafCount<List, Path, Depth>;

template <typename List, typename Path, int32_t Depth = 1>
constexpr int32_t LeafCountSup = LeafCount<List, Path, Depth> + SubtreeLeafCount<List, Path, Depth>;



namespace _ {

    template <typename T, T From, T To> struct MakeValueListH;

    template <typename T, T From, T To>
    struct MakeValueListH: HasType<
        MergeValueLists<
            ConstValue<T, From>,
            typename MakeValueListH<T, From + 1, To>::Type
        >
    >{};

    template <typename T, T To>
    struct MakeValueListH<T, To, To>: HasType<ValueList<T>> {};
}

template <typename T, T From, T To>
using MakeValueList = typename _::MakeValueListH<T, From, To>::Type;



template <int32_t V, typename VList> struct AddToValueListH;

template <
    int32_t V,
    typename T,
    T Head,
    T... Tail
>
struct AddToValueListH<V, ValueList<T, Head, Tail...>>: HasType<
    MergeValueLists<
            ValueList<T, V + Head>,
            typename AddToValueListH<V, ValueList<T, Tail...>>::Type
    >
>{};

template <int32_t V, typename T>
struct AddToValueListH<V, ValueList<T>>: HasType<ValueList<T>> {};


template <int32_t V, typename VList>
using AddToValueList = typename AddToValueListH<V, VList>::Type;





template <typename List, int32_t LeafIdx, typename Path = IntList<>, int32_t Idx = 0> struct BuildTreePath;

namespace details {

template <bool Condition, typename List, int32_t LeafIdx, typename Path, int32_t Idx> struct BuildTreePathHelper1;
template <bool Condition, typename List, int32_t LeafIdx, typename Path, int32_t Idx> struct BuildTreePathHelper2;

template <typename List, int32_t LeafIdx, int32_t... Path, int32_t Idx>
struct BuildTreePathHelper1<true, List, LeafIdx, IntList<Path...>, Idx>: HasType<
    MergeValueLists<IntList<Path...>, IntList<Idx>>
>{};

template <typename List, int32_t LeafIdx, int32_t... Path, int32_t Idx>
struct BuildTreePathHelper1<false, List, LeafIdx, IntList<Path...>, Idx>: HasType<
    typename BuildTreePath<
            List,
            LeafIdx - 1,
            IntList<Path...>,
            Idx + 1
    >::Type
>{};


template <typename... Head, typename... Tail, int32_t LeafIdx, int32_t... Path, int32_t Idx>
struct BuildTreePathHelper2<true, TypeList<TypeList<Head...>, Tail...>, LeafIdx, IntList<Path...>, Idx>: HasType< 
    typename BuildTreePath<
            TypeList<Head...>,
            LeafIdx,
            MergeValueLists<IntList<Path...>, IntList<Idx>>,
            0
    >::Type
>{};

template <typename... Head, typename... Tail, int32_t LeafIdx, int32_t... Path, int32_t Idx>
struct BuildTreePathHelper2<false, TypeList<TypeList<Head...>, Tail...>, LeafIdx, IntList<Path...>, Idx> {
    static const int32_t SubtreeSize = ListSize<Linearize<TypeList<Head...>>>;

    using Type = typename BuildTreePath<
            TypeList<Tail...>,
            LeafIdx - SubtreeSize,
            IntList<Path...>,
            Idx + 1
    >::Type;
};


}


template <
    typename... Head,
    typename... Tail,
    int32_t... PathList,
    int32_t LeafIdx,
    int32_t Idx
>
struct BuildTreePath<TypeList<TypeList<Head...>, Tail...>, LeafIdx, IntList<PathList...>, Idx> {
    static const int32_t SubtreeSize = ListSize<Linearize<TypeList<Head...>>>;

    using Type = typename details::BuildTreePathHelper2<
            (LeafIdx < SubtreeSize),
            TypeList<TypeList<Head...>, Tail...>,
            LeafIdx,
            IntList<PathList...>,
            Idx
    >::Type;
};


template <
    typename Head,
    typename... Tail,
    int32_t... PathList,
    int32_t LeafIdx,
    int32_t Idx
>
struct BuildTreePath<TypeList<Head, Tail...>, LeafIdx, IntList<PathList...>, Idx>: HasType<
    typename details::BuildTreePathHelper1<
            (LeafIdx < 1),
            TypeList<Tail...>,
            LeafIdx,
            IntList<PathList...>,
            Idx
    >::Type
>{};



template <
    int32_t... PathList,
    int32_t LeafIdx,
    int32_t Idx
>
struct BuildTreePath<TypeList<>, LeafIdx, IntList<PathList...>, Idx>;

}}
