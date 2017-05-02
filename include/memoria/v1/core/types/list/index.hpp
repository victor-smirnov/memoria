
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

namespace memoria {
namespace v1 {

template <typename Item_> struct DefaultItemProvider {
    typedef Item_ Item;
};

template <typename Item, typename List, int32_t Idx = 0> struct IndexOfTool;

template <typename Item, typename ... Tail, int32_t Idx>
struct IndexOfTool<Item, TypeList<Item, Tail...>, Idx> {
    static const int32_t Value = Idx;
};

template <typename Item, typename Head, typename ... Tail, int32_t Idx>
struct IndexOfTool<Item, TypeList<Head, Tail...>, Idx> {
    static const int32_t Value = IndexOfTool<Item, TypeList<Tail...>, Idx + 1>::Value;
};

template <typename Item, int32_t Idx>
struct IndexOfTool<Item, TypeList<>, Idx> {
    static const int32_t Value = -1; //not found
};




template <int32_t Idx, typename List, bool ReturnDefault = false, int32_t Counter = 0> struct SelectByIndexTool;

template <int32_t Idx, typename List, bool ReturnDefault = false>
using SelectByIndex = typename SelectByIndexTool<Idx, List, ReturnDefault>::Type;

template <int32_t idx> class ListIndexOutOfRange;


template <int32_t Idx, bool ReturnDefault, typename Head, typename ... Tail>
struct SelectByIndexTool<Idx, TypeList<Head, Tail...>, ReturnDefault, Idx> {
    using Type = Head;
};

template <int32_t Idx, bool ReturnDefault, typename Head, typename ... Tail, int32_t Counter>
struct SelectByIndexTool<Idx, TypeList<Head, Tail...>, ReturnDefault, Counter> {
    using Type = typename SelectByIndexTool<
                        Idx,
                        TypeList<Tail...>,
                        ReturnDefault,
                        Counter + 1
                     >::Type;
};

template <int32_t Idx, int32_t Counter>
struct SelectByIndexTool<Idx, TypeList<>, false, Counter>;
//{
//    using Type = ListIndexOutOfRange<Idx>;
//};

template <int32_t Idx, int32_t Counter>
struct SelectByIndexTool<Idx, TypeList<>, true, Counter> {
    using Type = NullType;
};


template <int32_t Idx, bool ReturnDefault, typename T, T Head, T ... Tail>
struct SelectByIndexTool<Idx, ValueList<T, Head, Tail...>, ReturnDefault, Idx> {
    static const T Value = Head;
};

template <int32_t Idx, bool ReturnDefault, typename T, T Head, T ... Tail, int32_t Counter>
struct SelectByIndexTool<Idx, ValueList<T, Head, Tail...>, ReturnDefault, Counter> {
    static const T Value = SelectByIndexTool<
                        Idx,
                        ValueList<T, Tail...>,
                        ReturnDefault,
                        Counter + 1
                     >::Value;
};

template <int32_t Idx, int32_t Counter, typename T>
struct SelectByIndexTool<Idx, ValueList<T>, false, Counter>;

template <int32_t Idx, int32_t Counter, typename T>
struct SelectByIndexTool<Idx, ValueList<T>, true, Counter> {
    static const T Value = 0;
};

}}