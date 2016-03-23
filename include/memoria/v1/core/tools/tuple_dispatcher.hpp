// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>

#include <tuple>

namespace memoria {
namespace tools     {

template <typename Tuple, Int Idx = std::tuple_size<Tuple>::value>
class TupleDispatcher {

    static const Int TupleSize  = std::tuple_size<Tuple>::value;
    static const Int ElementIdx = TupleSize - Idx;

public:

    template <typename Fn, typename... Args>
    void dispatch(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        fn.template operator()<ElementIdx>(std::get<ElementIdx>(tuple), args...);

        TupleDispatcher<Tuple, Idx - 1>::dispatch(tuple, std::forward<Fn>(fn), args...);
    }

    template <typename Fn, typename... Args>
    void dispatch(const Tuple& tuple, Fn&& fn, Args&&... args)
    {
        fn.template operator()<ElementIdx>(std::get<ElementIdx>(tuple), args...);

        TupleDispatcher<Tuple, Idx - 1>::dispatch(tuple, std::forward<Fn>(fn), args...);
    }

};


template <typename Tuple>
class TupleDispatcher<Tuple, 0> {
public:
    template <typename Fn, typename... Args>
    void dispatch(Tuple& tuple, Fn&& fn, Args&&... args) {

    }

    template <typename Fn, typename... Args>
    void dispatch(const Tuple& tuple, Fn&& fn, Args&&... args) {

    }
};

}
}
