
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

#include "btfl_create_test.hpp"
#include "btfl_seek_test.hpp"
#include "btfl_iterator_test.hpp"
#include "btfl_removal_test.hpp"

namespace memoria {
namespace v1 {
namespace tests {

namespace {

auto Suite1 = register_class_suite<BTFLCreateTest<BTFLTestCtr<2>>>("BTFL.Create.2");
auto Suite2 = register_class_suite<BTFLCreateTest<BTFLTestCtr<4>>>("BTFL.Create.4");

auto Suite3 = register_class_suite<BTFLSeekTest<BTFLTestCtr<2>>>("BTFL.Seek.2");
auto Suite4 = register_class_suite<BTFLSeekTest<BTFLTestCtr<4>>>("BTFL.Seek.4");

auto Suite5 = register_class_suite<BTFLIteratorTest<BTFLTestCtr<2>>>("BTFL.Iterator.2") ;
auto Suite6 = register_class_suite<BTFLIteratorTest<BTFLTestCtr<4>>>("BTFL.Iterator.4");

auto Suite7 = register_class_suite<BTFLRemoveTest<BTFLTestCtr<2>>>("BTFL.Remove.2");
auto Suite8 = register_class_suite<BTFLRemoveTest<BTFLTestCtr<4>>>("BTFL.Remove.4");

}

}}}
