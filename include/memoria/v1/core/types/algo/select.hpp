
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

#include <memoria/v1/core/types/list/typelist.hpp>
#include <memoria/v1/core/types/list/index.hpp>
#include <memoria/v1/core/types/list/append.hpp>
#include <memoria/v1/core/types.hpp>



namespace memoria {
namespace v1 {

namespace detail {

    template <int32_t Num, typename List, int32_t Idx> struct SelectT;

    template <int32_t Num, typename Head, typename ... Tail>
    struct SelectT<Num, TypeList<Head, Tail...>, Num>: HasType<Head> {};

    template <int32_t Num, typename Head, typename ... Tail, int32_t Idx>
    struct SelectT<Num, TypeList<Head, Tail...>, Idx>: HasType<
        typename SelectT<Num, TypeList<Tail...>, Idx + 1>::Type
    > {};

    template <int32_t Num, int32_t Idx>
    struct SelectT<Num, TypeList<>, Idx>;
}

template <int32_t Num, typename List>
using Select = typename memoria::v1::detail::SelectT<Num, List, 0>::Type;


namespace detail {

    template <int32_t Pos, typename List, int32_t idx = 0> struct SelectVH;

    template <int32_t Pos, typename T, T Head, T ... Tail>
    struct SelectVH<Pos, ValueList<T, Head, Tail...>, Pos> {
        static const T Value = Head;
    };

    template <int32_t Pos, typename T, T Head, T ... Tail, int32_t Idx>
    struct SelectVH<Pos, ValueList<T, Head, Tail...>, Idx> {
        static const T Value = SelectVH<Pos, ValueList<T, Tail...>, Idx + 1>::Value;
    };

    template <int32_t Value, typename T, int32_t Idx>
    struct SelectVH<Value, ValueList<T>, Idx>;
}

template <int32_t Pos, typename List> 
constexpr auto SelectV = memoria::v1::detail::SelectVH<Pos, List>::Value;





template <typename List, typename Default> struct SelectHeadIfNotEmpty;

template <typename Head, typename ... Tail, typename Default>
struct SelectHeadIfNotEmpty<TypeList<Head, Tail...>, Default> {
    typedef Head                                                                Result;
};

template <typename Default>
struct SelectHeadIfNotEmpty<TypeList<>, Default> {
    typedef Default                                                             Result;
};






template <bool Value, typename ResultIfTrue, typename Else>
struct IfThenElseT {
    using Result = typename Select<
                Value ? 0 : 1,
                TypeList<ResultIfTrue, Else>
    >::Result;
};

template <bool Value, typename ResultIfTrue, typename Else>
using IfThenElse = Select<
        Value ? 0 : 1,
        TypeList<ResultIfTrue, Else>
>;


template <typename Type1, typename Type2>
struct IfTypesEqual {
    static const bool Value = false;
};

template <typename Type>
struct IfTypesEqual<Type, Type> {
    static const bool Value = true;
};

}}
