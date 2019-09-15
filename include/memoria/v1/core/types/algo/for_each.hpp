
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/typelist.hpp>



namespace memoria {
namespace v1 {


template <
        typename Config,
        typename List,
        template <typename Config_, typename Item, typename LastResult> class Handler,
        typename Accumulator
>
struct ForEachItem;


template <
        typename Config,
        template <typename, typename, typename> class Handler,
        typename Accumulator
>
struct ForEachItem<Config, TypeList<>, Handler, Accumulator> {
    typedef Accumulator                                                         Result;
};

template <
        typename Config,
        typename Head,
        typename ... Tail,
        template <typename, typename, typename> class Handler,
        typename Accumulator
>
struct ForEachItem<Config, TypeList<Head, Tail...>, Handler, Accumulator> {
    typedef typename ForEachItem<
                Config,
                TypeList<Tail...>,
                Handler,
                Handler<Config, Head, Accumulator>
    >::Result                                                                   Result;
};



template <int32_t Idx, int32_t Size>
struct ForEach {
    template <typename Fn, typename... Args>
    static void process(Fn&& fn, Args&&... args)
    {
        if (fn.template process<Idx>(std::forward<Args>(args)...))
        {
            ForEach<Idx + 1, Size>::process(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }
};


template <int32_t Idx>
struct ForEach<Idx, Idx> {
    template <typename... Args>
    static void process(Args&&... args){}
};





}}
