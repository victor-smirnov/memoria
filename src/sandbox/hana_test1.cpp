
// Copyright 2016 Victor Smirnov
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


#include <boost/hana.hpp>

#include <memoria/v1/core/types/typelist.hpp>

#include <sstream>
#include <iostream>
#include <vector>

using namespace std;
namespace hana = boost::hana;

using namespace memoria::v1;
using namespace hana::literals;

template <typename T> struct PartialStruct;

template <>
struct PartialStruct<int> {};

//template <>
//struct PartialStruct<long> {};

int main() {
    constexpr auto ints = hana::tuple_c<int, 1, 2, 3, 2, 2, 4, 2>;
    BOOST_HANA_CONSTANT_CHECK(hana::count(ints, hana::int_c<2>) == hana::size_c<4>);
    static_assert(hana::count(ints, 2) == 4, "");
    constexpr auto types = hana::tuple_t<int, char, long, short, char, double>;
    BOOST_HANA_CONSTANT_CHECK(hana::count(types, hana::type_c<char>) == hana::size_c<2>);

    constexpr auto xs = hana::tuple_c<int, 1, 2, 3, 4, 5>;
    constexpr auto vs = hana::transform(xs, hana::value_of);
    static_assert(vs == hana::make_tuple(1, 2, 3, 4, 5), "");

    auto i = hana::integral_c<int, 3>;

    TypesPrinter<
        decltype(ints),
        decltype(hana::size_c<2>),
        decltype(types),
        decltype(hana::tuple_c<int, 1, 2, 3, 4, 5>),
        decltype(hana::make_tuple(1, 2, 3, 4, 5)),
        decltype(i)
    >
    ::print(std::cout) << endl;

    cout<<hana::value<decltype(i)>() << endl;
    cout<<hana::value(i) << endl;

    auto res = hana::if_(
        1_c,
        []{return hana::type_c<PartialStruct<int>>;},
        []{return hana::type_c<PartialStruct<long>>;}
    )();

    TypesPrinter<decltype(res)::type>::print(std::cout) << endl;
}
