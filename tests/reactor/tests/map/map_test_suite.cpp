
// Copyright 2012 Victor Smirnov
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


#include "map_test.hpp"

namespace memoria {
namespace tests {

namespace {

auto Suite1 = register_class_suite<MapTest<UID256, UID256, UID256, UID256>>("Map.UID256");
auto Suite2 = register_class_suite<MapTest<Varchar, Varchar, U8String, U8String>>("Map.Varchar");

}

}}
