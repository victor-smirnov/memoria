
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/types/algo/fold.hpp>
#include <memoria/v1/core/types/list/append.hpp>

namespace memoria {
namespace v1 {

namespace {
    template <typename State, typename Item, template <typename> class Fn>
    struct MapFoldFn {
        using Type = AppendToList<
                typename Fn<Item>::Type,
                State
        >;
    };

    template <template <typename> class Fn>
    struct MapFoldFn<EmptyType, EmptyType, Fn> {
        using Type = TL<>;
    };

    template <typename Element, template <typename> class Fn>
    struct MapFoldFn<EmptyType, Element, Fn> {
        using Type = TL<typename Fn<Element>::Type>;
    };

    template <typename List, template <typename> class MapFn>
    struct MapTLT {
        template <typename State, typename Item>
        using FoldFn = typename MapFoldFn<State, Item, MapFn>::Type;

        using Type   = FoldTLLeft<List, FoldFn>;
    };
}


template <typename List, template <typename> class MapFn>
using MapTL2 = typename MapTLT<List, MapFn>::Type;


}}
