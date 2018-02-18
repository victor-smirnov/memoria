
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/typelist.hpp>

namespace memoria {
namespace v1 {

namespace {

    template <typename List, template <typename, typename> class FoldingFn, typename State = EmptyType> struct FoldTLRightT;

    template <typename Head, typename... Tail, template <typename, typename> class FoldingFn, typename State>
    struct FoldTLRightT<TL<Head, Tail...>, FoldingFn, State>
    {
        using NewState  = FoldingFn<State, Head>;
        using Type      = typename FoldTLRightT<TL<Tail...>, FoldingFn, NewState>::Type;
    };

    template <typename Element, template <typename, typename> class FoldingFn, typename State>
    struct FoldTLRightT<TL<Element>, FoldingFn, State>
    {
        using Type = FoldingFn<State, Element>;
    };

    template <template <typename, typename> class FoldingFn, typename State>
    struct FoldTLRightT<TL<>, FoldingFn, State>
    {
        using Type = FoldingFn<State, EmptyType>;
    };



    template <typename List, template <typename, typename> class FoldingFn> struct FoldTLLeftT;

    template <typename Head, typename... Tail, template <typename, typename> class FoldingFn>
    struct FoldTLLeftT<TL<Head, Tail...>, FoldingFn>
    {
        using Type = FoldingFn<typename FoldTLLeftT<TL<Tail...>, FoldingFn>::Type, Head>;
    };

    template <typename Head, template <typename, typename> class FoldingFn>
    struct FoldTLLeftT<TL<Head>, FoldingFn>
    {
        using Type = FoldingFn<EmptyType, Head>;
    };

    template <template <typename, typename> class FoldingFn>
    struct FoldTLLeftT<TL<>, FoldingFn>
    {
        using Type = FoldingFn<EmptyType, EmptyType>;
    };

}

template <typename List, template <typename, typename> class FoldingFn, typename State = EmptyType>
using FoldTLRight = typename FoldTLRightT<List, FoldingFn, State>::Type;

template <typename List, template <typename, typename> class FoldingFn>
using FoldTLLeft = typename FoldTLLeftT<List, FoldingFn>::Type;

}}