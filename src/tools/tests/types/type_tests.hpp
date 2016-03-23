
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/types/list/misc.hpp>

#include <type_traits>


namespace memoria {
namespace v1 {
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
}}