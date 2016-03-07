
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/hana.hpp>

#include <memoria/core/types/typelist.hpp>

#include <sstream>
#include <iostream>
#include <vector>

using namespace std;
namespace hana = boost::hana;

using namespace memoria;


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
    ::print(std::cout)<<std::endl;

    cout<<hana::value<decltype(i)>()<<endl;
    cout<<hana::value(i)<<endl;
}
