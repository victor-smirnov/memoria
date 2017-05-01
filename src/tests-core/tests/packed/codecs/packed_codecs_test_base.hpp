
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


#pragma once

#include <memoria/v1/tests/tests_inc.hpp>


namespace memoria {
namespace v1 {

using namespace std;

template <typename ValueT>
class PackedCodecsTestBase: public TestTask {
    using Base = TestTask;
protected:

    static constexpr Int MEMBUF_SIZE = 1024*1024*64;


    using Value     = ValueT;

public:

    using Base::getRandom;

    PackedCodecsTestBase(StringRef name): TestTask(name)
    {}

};

}}
