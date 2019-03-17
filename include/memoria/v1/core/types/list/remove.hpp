
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
#include <memoria/v1/core/types/list/append.hpp>

namespace memoria {
namespace v1 {

namespace _ {
    
    template <typename Item, typename List, bool All = false> struct RemoveToolH;

    template <typename Item, typename Head, typename ... Tail, bool All>
    struct RemoveToolH<Item, TypeList<Head, Tail...>, All>: HasType<
        MergeLists<
            TL<Head>,
            typename RemoveToolH<
                        Item,
                        TypeList<Tail...>,
                        All
            >::Type
        >
    >{};

    template <typename Item, typename ... Tail>
    struct RemoveToolH<Item, TypeList<Item, Tail...>, false>: HasType<TL<Tail...>> {};

    template <typename Item, typename ... Tail>
    struct RemoveToolH<Item, TypeList<Item, Tail... >, true>: HasType< 
        typename RemoveToolH<
            Item,
            TypeList<Tail...>,
            true
        >::Type
    > {};


    template <typename Item, bool All>
    struct RemoveToolH<Item, TypeList<>, All>: HasType<TL<>> {};




    template <typename List> struct RemoveDuplicatesToolH;

    template<>
    struct RemoveDuplicatesToolH<TypeList<>>: HasType<TL<>> {};



    template <typename Head, typename ... Tail>
    struct RemoveDuplicatesToolH<TypeList<Head, Tail...> > {
    private:
        using TailResult = typename RemoveDuplicatesToolH<TypeList<Tail...>>::Type;
        using HeadResult = typename RemoveToolH<Head, TailResult>::Type;
    public:
        using Type = MergeLists<TL<Head>, HeadResult>;
    };
}

template <typename Item, typename List, bool All = false> 
using RemoveTool = typename _::RemoveToolH<Item, List, All>::Type;

template <typename List> 
using RemoveDuplicatesTool = typename _::RemoveDuplicatesToolH<List>::Type;

}}
