
// Copyright 2019 Victor Smirnov
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
#include <boost/hana.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

template <typename TL> struct ToHana;

template <typename... Type>
struct ToHana<TL<Type...>>
{
    static constexpr auto cvt() {
        return boost::hana::tuple_t<Type...>;
    }
};


namespace _ {
    template <typename Tuple> struct FromHanaH;

    template <typename... Type>
    struct FromHanaH<boost::hana::tuple<Type ...>>
    {
        static constexpr auto cvt() {
            return TL<typename Type::type ...>{};
        }
    };
}


template <typename TL>
constexpr auto to_hana() {
    return ToHana<TL>::cvt();
}

template <typename Tuple>
constexpr auto from_hana(Tuple&& v) {
    return _::FromHanaH<std::decay_t<decltype(v)>>::cvt();
}


}}
