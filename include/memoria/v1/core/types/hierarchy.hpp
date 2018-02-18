
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

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/config.hpp>

namespace memoria {
namespace v1 {

using v1::TypeList;

template <
    typename TChain,
    template <typename, typename> class Element,
    typename Base = EmptyType
>
struct SimpleHierarchy;

template <
    typename T1,
    typename ... T2,
    template <typename, typename> class Element,
    typename Base
>
struct SimpleHierarchy<TypeList<T1, T2...>, Element, Base>:
        public Element<T1, SimpleHierarchy<TypeList <T2...>, Element, Base> > {};

template <
    typename T,
    template <typename, typename> class Element,
    typename Base
>
struct SimpleHierarchy<TypeList<T>, Element, Base>: public Element<T, Base> {};

}}