
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




#include "multimap_basic_test.hpp"

#include <vector>

namespace memoria {
namespace v1 {
namespace tests {
namespace {

//auto Suite1 = register_class_suite<MultiMapBasicTest<Multimap<U8String, int64_t>>>("Miltimap.Basic.S.I");
//auto Suite2 = register_class_suite<MultiMapBasicTest<Multimap<double, int64_t>>>("Miltimap.Basic.D.I");
auto Suite3 = register_class_suite<MultiMapBasicTest<Multimap<int64_t, uint8_t>>>("Miltimap.Basic.I.B");

//auto Suite4 = register_class_suite<MultiMapBasicTest<Multimap<UUID, int64_t>>>("Miltimap.Basic.U.I");
//auto Suite5 = register_class_suite<MultiMapBasicTest<Multimap<UUID, U8String>>>("Miltimap.Basic.U.S");




}
}}}
