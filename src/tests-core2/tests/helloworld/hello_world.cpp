
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/tests/tests.hpp>

namespace memoria {
namespace v1 {
namespace tests {




auto test1 = register_test<FnTest<>>(u"Crashed-HelloWorldTest", [](auto&){
    int* a = nullptr;
    *a = 12345;
});

auto test2 = register_test<FnTest<>>(u"Failed-HelloWorldTest", [](auto&){
    MMA1_THROW(TestException()) << WhatCInfo("Something happened");
});

auto test3 = register_test<FnTest<>>(u"Passed-HelloWorldTest", [](auto&){

});

}}}
