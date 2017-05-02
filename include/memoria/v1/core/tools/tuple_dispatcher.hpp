// Copyright 2013 Victor Smirnov
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

#include <tuple>

namespace memoria {
namespace v1 {
namespace tools     {

template <typename Tuple, int32_t Idx = std::tuple_size<Tuple>::value>
class TupleDispatcher {

    static const int32_t TupleSize  = std::tuple_size<Tuple>::value;
    static const int32_t ElementIdx = TupleSize - Idx;

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
}}