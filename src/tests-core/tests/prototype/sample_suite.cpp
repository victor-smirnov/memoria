
// Copyright 2013 Victor Smirnov
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>
#include <memoria/tests/yaml.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <memoria/api/allocator/allocator_inmem_api.hpp>

namespace memoria {
namespace tests {

using namespace reactor;

class SampleSuite: public TestState {
    using MyType = SampleSuite;
    using Base = TestState;

    StdString text_{"ABCDE"};

    InMemAllocator<> allocator_;

    UUID uuid_{UUID::make_random()};

public:

    MMA_STATE_FILEDS(text_, uuid_);
    MMA_INDIRECT_STATE_FILEDS(allocator_);

    SampleSuite() {
        allocator_ = InMemAllocator<>::create();
    }

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TEST_WITH_REPLAY(suite, doSomething, replaySomething);
    }

    void doSomething()
    {
        text_ += ": 12345 99";
        engine().coutln("Test doSomething: {}", text_);
        //assert_equals(1,2);
    }

    void replaySomething()
    {
        engine().coutln("Replay doSomething: {} {}", text_, uuid_);
    }
};



namespace {
auto Suite1 = register_class_suite<SampleSuite>("SampleSuite");
}


}}
