
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

#include <memoria/core/types.hpp>
#include <boost/mp11.hpp>

#include <type_traits>

namespace memoria {

namespace _ {
    template <typename T> struct ToMp11H;

    template <typename... Type>
    struct ToMp11H<TL<Type...>>: HasType<boost::mp11::mp_list<Type...>> {};

    template <typename T> struct FromMp11H;

    template <typename... Type>
    struct FromMp11H<boost::mp11::mp_list<Type...>>: HasType<TL<Type...>> {};
}




template <typename T>
using ToMp11 = typename _::ToMp11H<T>::Type;

template <typename T>
using FromMp11 = typename _::FromMp11H<T>::Type;

}
