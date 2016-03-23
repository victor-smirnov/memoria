
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

template <typename Item, typename List, Int Idx = 0> struct IndexOfTool;

template <typename Item, typename ... Tail, Int Idx>
struct IndexOfTool<Item, TypeList<Item, Tail...>, Idx> {
    static const Int Value = Idx;
};

template <typename Item, typename Head, typename ... Tail, Int Idx>
struct IndexOfTool<Item, TypeList<Head, Tail...>, Idx> {
    static const Int Value = IndexOfTool<Item, TypeList<Tail...>, Idx + 1>::Value;
};

template <typename Item, Int Idx>
struct IndexOfTool<Item, TypeList<>, Idx> {
    static const Int Value = -1; //not found
};




template <Int Idx, typename List, bool ReturnDefault = false, Int Counter = 0> struct SelectByIndexTool;

template <Int Idx, typename List, bool ReturnDefault = false>
using SelectByIndex = typename SelectByIndexTool<Idx, List, ReturnDefault>::Type;

template <Int idx> class ListIndexOutOfRange;


template <Int Idx, bool ReturnDefault, typename Head, typename ... Tail>
struct SelectByIndexTool<Idx, TypeList<Head, Tail...>, ReturnDefault, Idx> {
    using Type = Head;
};

template <Int Idx, bool ReturnDefault, typename Head, typename ... Tail, Int Counter>
struct SelectByIndexTool<Idx, TypeList<Head, Tail...>, ReturnDefault, Counter> {
    using Type = typename SelectByIndexTool<
                        Idx,
                        TypeList<Tail...>,
                        ReturnDefault,
                        Counter + 1
                     >::Type;
};

template <Int Idx, Int Counter>
struct SelectByIndexTool<Idx, TypeList<>, false, Counter>;
//{
//    using Type = ListIndexOutOfRange<Idx>;
//};

template <Int Idx, Int Counter>
struct SelectByIndexTool<Idx, TypeList<>, true, Counter> {
    using Type = NullType;
};


template <Int Idx, bool ReturnDefault, typename T, T Head, T ... Tail>
struct SelectByIndexTool<Idx, ValueList<T, Head, Tail...>, ReturnDefault, Idx> {
    static const T Value = Head;
};

template <Int Idx, bool ReturnDefault, typename T, T Head, T ... Tail, Int Counter>
struct SelectByIndexTool<Idx, ValueList<T, Head, Tail...>, ReturnDefault, Counter> {
    static const T Value = SelectByIndexTool<
                        Idx,
                        ValueList<T, Tail...>,
                        ReturnDefault,
                        Counter + 1
                     >::Value;
};

template <Int Idx, Int Counter, typename T>
struct SelectByIndexTool<Idx, ValueList<T>, false, Counter>;

template <Int Idx, Int Counter, typename T>
struct SelectByIndexTool<Idx, ValueList<T>, true, Counter> {
    static const T Value = 0;
};

}}