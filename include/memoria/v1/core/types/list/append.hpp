
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/types/list/typelist.hpp>

namespace memoria {
namespace v1 {

namespace detail {

    template <typename Item1, typename Item2> struct AppendToolH;

    template <typename Item, typename ... List>
    struct AppendToolH<Item, TypeList<List...> > {
        using Type = TypeList<Item, List...>;
    };

    template <typename ... List1, typename ... List2>
    struct AppendToolH<TypeList<List1...>, TypeList<List2...> > {
        using Type = TypeList<List1..., List2...>;
    };

    template <typename ... List, typename Item>
    struct AppendToolH<TypeList<List...>, Item > {
        using Type = TypeList<List..., Item>;
    };

    template <typename T, T ... List1, T ... List2>
    struct AppendToolH<ValueList<T, List1...>, ValueList<T, List2...> > {
        using Type = ValueList<T, List1..., List2...>;
    };

    // Errors:
    template <typename ... List>
    struct AppendToolH<TypeList<List...>, NullType>;

    template <typename ... List>
    struct AppendToolH<NullType, TypeList<List...>>;

    template <>
    struct AppendToolH<NullType, NullType>;

    
    
    
    
    
    template <typename Accumulator, typename ... Lists> class MergeTypeListsHelper;

    template <typename Accumulator, typename ... List, typename ... Tail>
    class MergeTypeListsHelper<Accumulator, TypeList<List...>, Tail...> {
        using R0 = typename AppendToolH<Accumulator, TypeList<List...>>::Type;
    public:
        using Type = typename MergeTypeListsHelper<R0, Tail...>::Type;
    };

    template <typename Accumulator, typename Item, typename ... Tail>
    class MergeTypeListsHelper<Accumulator, Item, Tail...> {
        using R0 = typename AppendToolH<Accumulator, TypeList<Item>>::Type;
    public:
        using Type = typename MergeTypeListsHelper<R0, Tail...>::Type;
    };


    template <typename Accumulator>
    class MergeTypeListsHelper<Accumulator> {
    public:
        using Type = Accumulator;
    };



    template <typename Accumulator, typename ... Lists> class MergeValueListsHelper;

    template <typename Accumulator, typename T, T... List, typename ... Tail>
    class MergeValueListsHelper<Accumulator, ValueList<T, List...>, Tail...> {
        using R0 = typename AppendToolH<Accumulator, ValueList<T, List...>>::Type;
    public:
        using Type = typename MergeValueListsHelper<R0, Tail...>::Type;
    };
    

    template <typename Accumulator, typename T, T Value, typename ... Tail>
    class MergeValueListsHelper<Accumulator, ConstValue<T, Value>, Tail...> {
        using R0 = typename AppendToolH<Accumulator, ValueList<T, Value>>::Type;
    public:
        using Type = typename MergeValueListsHelper<R0, Tail...>::Type;
    };

    template <typename Accumulator>
    class MergeValueListsHelper<Accumulator> {
    public:
        using Type = Accumulator;
    };

    
    
    
    
    template <typename ... Lists> struct MergeValueListsH;

    template <
        typename T,
        T... List,
        typename... Tail
    >
    struct MergeValueListsH<ValueList<T, List...>, Tail...> {
        using Type = typename MergeValueListsHelper<ValueList<T, List...>, Tail...>::Type;
    };


    template <
        typename T,
        T Value,
        T... List,
        typename... Tail
    >
    struct MergeValueListsH<ValueList<T, List...>, ConstValue<T, Value>, Tail...> {
    private:
        using R0 = typename MergeValueListsHelper<ValueList<T, List...>, ValueList<T, Value>>::Type;

    public:
        using Type = typename MergeValueListsH<R0, Tail...>::Type;
    };


    template <
        typename T,
        T Value,
        typename... Tail
    >
    struct MergeValueListsH<ConstValue<T, Value>, Tail...> {
        using Type = typename MergeValueListsH<ValueList<T, Value>, Tail...>::Type;
    };
    
    template <typename ... Lists> struct MergeListsH {
        using Type = typename detail::MergeTypeListsHelper<TypeList<>, Lists...>::Type;
    };
    
    template <typename List, typename Item, int32_t Idx> struct ReplaceH;
    
    template <typename Head, typename... Tail, typename Item>
    struct ReplaceH<TypeList<Head, Tail...>, Item, 0> {
        using Type = TypeList<Item, Tail...>;
    };

    template <typename Head, typename... Tail, typename Item, int32_t Idx>
    struct ReplaceH<TypeList<Head, Tail...>, Item, Idx> {
        using Type = typename MergeListsH<
                TypeList<Head>,
                typename ReplaceH<TypeList<Tail...>, Item, Idx - 1>::Type
        >::Type;
    };
    

    template <typename T, T Item1, typename List> struct AppendValueToolH;
    // ValueList
    template <typename T, T Item, T ... List>
    struct AppendValueToolH<T, Item, ValueList<T, List...> > {
        using Type = ValueList<T, Item, List...>;
    };

}


template <typename Item1, typename Item2> 
using AppendTool = typename detail::AppendToolH<Item1, Item2>::Type;


template <typename ... Lists>
using MergeLists = typename detail::MergeListsH<Lists...>::Type;



template <typename ... Lists>
using MergeValueLists = typename detail::MergeValueListsH<Lists...>::Type;


template <typename List, typename Item, int32_t Idx>
using Replace = typename detail::ReplaceH<List, Item, Idx>::Type;

template <typename T, T Item1, typename List> 
using AppendValueTool = typename detail::AppendValueToolH<T, Item1, List>::Type;



}}
