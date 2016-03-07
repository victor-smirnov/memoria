
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TYPE_TESTS_HPP_
#define MEMORIA_TESTS_TYPE_TESTS_HPP_

#include <memoria/core/types/list/misc.hpp>

#include <type_traits>


namespace memoria   {
namespace tests     {

using namespace std;

namespace same_type {



class T {};

class T1 {};
class T2 {};

using List1 = TL<T1, T2, T1>;
using List2 = TL<T1, T2, T1>;
using List3 = TL<T1, T2, T>;

static_assert(is_same<T, T>(), "");
static_assert(!is_same<T1, T2>(), "");

static_assert(is_same<List1, List2>(), "");
static_assert(!is_same<List1, List3>(), "");

static_assert(IntValue<10>{} == 10, "");



}

namespace {
class T{};
}




}
}



#endif
