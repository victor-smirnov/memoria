
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

#include <memoria/v1/core/config.hpp>
#include <memoria/v1/core/types/list/append.hpp>

namespace memoria {
namespace v1 {


template <typename List, int32_t Len> struct SublistFromStart;

template <typename T, T Head, T... Tail, int32_t Len>
struct SublistFromStart<ValueList<T, Head, Tail...>, Len> {
    static_assert(Len >= 0, "Len parameter must be >= 0");
    static_assert(Len <= sizeof...(Tail) + 1, "Len parameter must be <= the Length of value list");

    using Type = MergeValueLists<
                ConstValue<T, Head>,
                typename SublistFromStart<ValueList<T, Tail...>, Len - 1>::Type
    >;
};



template <typename T, T Head, T... Tail>
struct SublistFromStart<ValueList<T, Head, Tail...>, 0> {
    using Type = ValueList<T>;
};

template <typename T>
struct SublistFromStart<ValueList<T>, 0> {
    using Type = ValueList<T>;
};




template <typename Head, typename... Tail, int32_t Len>
struct SublistFromStart<TypeList<Head, Tail...>, Len> {
    static_assert(Len >= 0, "Len parameter must be >= 0");
    static_assert(Len <= sizeof...(Tail) + 1, "Len parameter must be <= the Length of value list");

    using Type = MergeLists<
                Head,
                typename SublistFromStart<TypeList<Tail...>, Len - 1>::Type
    >;
};

template <typename Head, typename... Tail>
struct SublistFromStart<TypeList<Head, Tail...>, 0> {
    using Type = TypeList<>;
};

template <>
struct SublistFromStart<TypeList<>, 0> {
    using Type = TypeList<>;
};



template <typename List, int32_t From> struct SublistToEnd;

template <typename T, T Head, T... Tail>
struct SublistToEnd<ValueList<T, Head, Tail...>, 0> {
    using Type = ValueList<T, Head, Tail...>;
};

template <typename T>
struct SublistToEnd<ValueList<T>, 0> {
    using Type = ValueList<T>;
};


template <typename T, T Head, T ... Tail, int32_t From>
struct SublistToEnd<ValueList<T, Head, Tail...>, From> {
    static_assert(From >= 0, "Form must be >= 0");
    static_assert(From < sizeof...(Tail) + 1, "Form must be <= length of the list");

    using Type = typename SublistToEnd<ValueList<T, Tail...>, From - 1>::Type;
};



template <typename Head, typename... Tail>
struct SublistToEnd<TypeList<Head, Tail...>, 0> {
    using Type = TypeList<Head, Tail...>;
};

template <>
struct SublistToEnd<TypeList<>, 0> {
    using Type = TypeList<>;
};

template <typename Head, typename... Tail, int32_t From>
struct SublistToEnd<TypeList<Head, Tail...>, From> {
    static_assert(From >= 0, "Form must be >= 0");
    static_assert(From <= sizeof...(Tail) + 1, "Form must be <= length of the list");

    using Type = typename SublistToEnd<TypeList<Tail...>, From - 1>::Type;
};



template <typename List, int32_t From, int32_t To>
struct Sublist {
    static_assert(From <= To, "Form must be <= To");

    using Type = typename SublistFromStart<
            typename SublistToEnd<List, From>::Type,
            To - From
    >::Type;
};

}}
