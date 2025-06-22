
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
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/result.hpp>

#include <fmt/format.h>

namespace memoria {

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


template <size_t Idx>
struct ForEachIdx {
    constexpr size_t value() const {
        return Idx;
    }

    constexpr operator size_t() const {
        return Idx;
    }
};


template <size_t Idx, size_t Size>
struct ForEach {

    template <typename Fn, typename... Args>
    static void process(Fn&& fn, Args&&... args){
        if (fn.template process<Idx>(std::forward<Args>(args)...)){
            ForEach<Idx + 1, Size>::process(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    template <typename Fn, typename... Args>
    static VoidResult process_res(Fn&& fn, Args&&... args) noexcept {
        MEMORIA_TRY(proceed_next, fn.template process<Idx>(std::forward<Args>(args)...));

        if (proceed_next) {
            return ForEach<Idx + 1, Size>::process_res(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }

        return VoidResult::of();
    }


    template <typename Fn, typename... Args>
    static void process_fn(Fn&& fn, Args&&... args)
    {
        fn(ForEachIdx<Idx>{}, std::forward<Args>(args)...);
        ForEach<Idx + 1, Size>::process_fn(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    static VoidResult process_res_fn(Fn&& fn, Args&&... args) noexcept
    {
        MEMORIA_TRY_VOID(fn(ForEachIdx<Idx>{}, std::forward<Args>(args)...));
        return ForEach<Idx + 1, Size>::process_res_fn(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};


template <size_t Idx>
struct ForEach<Idx, Idx> {
    template <typename... Args>
    static void process(Args&&... args){}

    template <typename... Args>
    static VoidResult process_res(Args&&... args) noexcept {
        return VoidResult::of();
    }

    template <typename... Args>
    static void process_fn(Args&&... args){}

    template <typename... Args>
    static VoidResult process_res_fn(Args&&... args) noexcept {
        return VoidResult::of();
    }
};



template <int32_t Idx, int32_t Size>
struct ForOneOf {
    template <typename Fn, typename... Args>
    static auto process(int32_t stream, Fn&& fn, Args&&... args){
        if (Idx == stream)
        {
            return fn.template process<Idx>(std::forward<Args>(args)...);
        }
        else {
            return ForEach<Idx + 1, Size>::process(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }
};


template <int32_t Idx>
struct ForOneOf<Idx, Idx> {
    template <typename Fn, typename... Args>
    static auto process(int32_t stream, Fn&& fn, Args&&... args){
        if (Idx == stream)
        {
            return fn.template process<Idx>(std::forward<Args>(args)...);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("").do_throw();
        }
    }
};

}

namespace fmt {

template <size_t Idx>
struct formatter<memoria::ForEachIdx<Idx>> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::ForEachIdx<Idx>& d, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", d.value());
    }
};

}
