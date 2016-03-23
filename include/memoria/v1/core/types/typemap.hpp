
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