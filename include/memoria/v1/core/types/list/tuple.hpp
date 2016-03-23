
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

#include <memoria/v1/core/types/types.hpp>

#include <tuple>

namespace memoria {
namespace v1 {


template <typename T, Int Size>
struct MakeList {
    using Type = MergeLists<
                T,
                typename MakeList<T, Size - 1>::Type
    >;
};

template <typename T>
struct MakeList<T, 0> {
    using Type = TypeList<>;
};

template <typename T> struct MakeTupleH;

template <typename... List>
struct MakeTupleH<TypeList<List...>> {
    using Type = std::tuple<List...>;
};

template <typename T, Int Size>
using MakeTuple = typename MakeTupleH<typename MakeList<T, Size>::Type>::Type;


}}