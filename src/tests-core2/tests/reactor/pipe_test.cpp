
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

#include <memoria/v1/reactor/reactor.hpp>

namespace memoria {
namespace v1 {
namespace tests {

struct PipeTestState: TestState {
    using Base = TestState;

    int size{256*1024};

    MMA1_STATE_FILEDS(size)

};


auto test = register_test_in_suite<FnTest<PipeTestState>>(u"ReactorSuite", u"PipeTest", [](auto& state){
    for (int c = 0; c < 100000; c++)
    {
        reactor::engine().coutln(u"THREADS: {} {} {}", c, reactor::engine().cpu_num(), state.working_directory_);
    }
});

}}}
