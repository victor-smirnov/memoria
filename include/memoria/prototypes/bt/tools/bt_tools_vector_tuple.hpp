
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/static_array.hpp>


#include <tuple>
#include <ostream>

namespace memoria {

namespace internal {

template <int32_t Idx>
struct DoRecursive {
    template <typename Fn>
    static void process(Fn&& fn)
    {
        DoRecursive<Idx - 1>::process(std::forward<Fn>(fn));

        fn.template operator()<Idx - 1>();
    }
};

template <>
struct DoRecursive<0> {
    template <typename Fn>
    static void process(Fn&& fn) {}
};


template <typename... Args>
struct OstreamFn {
    typedef std::tuple<Args...> Tuple;
    std::ostream& out_;
    const Tuple& obj_;

    OstreamFn(std::ostream& out, const Tuple& obj): out_(out), obj_(obj) {}

    template <int32_t Idx>
    void operator()()
    {
        out_<<std::get<Idx>(obj_);

        if (Idx < sizeof...(Args) - 1)
        {
            out_<<", ";
        }
    }
};

}

}

namespace std {

template <typename... Args>
std::ostream& operator<<(std::ostream& out, const std::tuple<Args...>& obj)
{
    out << "{";
    memoria::internal::DoRecursive<sizeof...(Args)>::process(memoria::internal::OstreamFn<Args...>(out, obj));
    out << "}";
    return out;
}

}
