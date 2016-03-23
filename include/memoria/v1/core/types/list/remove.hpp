
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

template <typename Item, typename List, bool All = false> struct RemoveTool;

template <typename Item, typename Head, typename ... Tail, bool All>
struct RemoveTool<Item, TypeList<Head, Tail...>, All> {
    typedef typename AppendTool<
                        Head,
                        typename RemoveTool<
                                    Item,
                                    TypeList<Tail...>,
                                    All
                        >::Result
                    >::Result                                                   Result;
};

template <typename Item, typename ... Tail>
struct RemoveTool<Item, TypeList<Item, Tail...>, false> {
    typedef TypeList<Tail...>                                                   Result;
};

template <typename Item, typename ... Tail>
struct RemoveTool<Item, TypeList<Item, Tail... >, true> {
    typedef typename RemoveTool<
                        Item,
                        TypeList<Tail...>,
                        true
                    >::Result                                                   Result;
};


template <typename Item, bool All>
struct RemoveTool<Item, TypeList<>, All> {
    typedef TypeList<>                                                          Result;
};




template <typename List> struct RemoveDuplicatesTool;


template<>
struct RemoveDuplicatesTool<TypeList<>> {
    typedef TypeList<>                                                          Result;
};



template <typename Head, typename ... Tail>
struct RemoveDuplicatesTool<TypeList<Head, Tail...> > {
private:
    typedef typename RemoveDuplicatesTool<TypeList<Tail...>>::Result    TailResult;
    typedef typename RemoveTool<Head, TailResult>::Result       HeadResult;
public:
    typedef typename AppendTool<Head, HeadResult>::Result                       Result;
};

}}