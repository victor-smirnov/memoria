
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/core/strings/strings.hpp>

namespace memoria {
namespace tests {


template <typename ValueT>
class PackedCodecsTestBase: public TestState {
    using Base = TestState;
protected:

    static constexpr int32_t MEMBUF_SIZE = 1024*1024*64;
    using Value = ValueT;
};

}}
