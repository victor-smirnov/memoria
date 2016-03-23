
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/types/algo/fold.hpp>
#include <memoria/v1/core/types/list/append.hpp>

namespace memoria {
namespace v1 {

namespace details {
    template <typename State, typename Item, typename Fn>
    struct MapFoldFn {
        using Type = AppendItemToList<typename Fn<Item>::Type, typename State::Type>;
    };

    template <typename Fn>
    struct MapFoldFn<EmptyType, EmptyType, Fn> {
        using Type = TL<>;
    };

    template <typename Element, typename Fn>
    struct MapFoldFn<EmptyType, Element, Fn> {
        using Type = TL<Element>;
    };
}

template <typename List, template <typename> class MapFn> struct MapTL;

template <typename Head, template <typename> class MapFn>
struct MapTL_T {
    template <typename State, typename Item>
    using FoldFn = typename details::MapFoldFn<State, Item>::Type;

    using Type = typename FoldRight<List, FoldFn>::Type::Type;
};

}}