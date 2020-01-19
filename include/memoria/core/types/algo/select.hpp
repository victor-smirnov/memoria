
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

#include <memoria/core/types.hpp>

#include <memoria/core/types/mp11.hpp>


namespace memoria {

template <int32_t Num, typename List>
using Select = boost::mp11::mp_at_c<List, Num>;

namespace _ {

    template <int32_t Pos, typename List>
    struct SelectVH;

    template <int32_t Pos, typename T, T... Values>
    struct SelectVH<Pos, ValueList<T, Values...>> {
        static constexpr T get() {
            T array[] = {Values...};
            return array[Pos];
        }
    };

    template <int32_t Pos, typename List>
    struct SelectVHD;

    template <int32_t Pos, typename T, T... Values>
    struct SelectVHD<Pos, ValueList<T, Values...>> {
        static constexpr T get() {
            T array[] = {Values...};
            return Pos >= 0 && Pos < sizeof...(Values) ? array[Pos] : T();
        }
    };
}

template <int32_t Pos, typename List> 
constexpr auto SelectV = memoria::_::SelectVH<Pos, List>::get();

template <int32_t Pos, typename List>
constexpr auto SelectVOrDefault = memoria::_::SelectVHD<Pos, List>::get();


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

}
