
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>

#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>



namespace memoria {
namespace v1 {
namespace tests {

using namespace reactor;

class SampleSuite: public TestState {
    using MyType = SampleSuite;
    using Base = TestState;

    StdString text_{"ABCDE"};

public:

    MMA1_STATE_FILEDS(text_);

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TEST_WITH_REPLAY(suite, doSomething, replaySomething);
    }

    void doSomething()
    {
        text_ += ": 12345 99";
        engine().coutln(u"Test doSomething: {}", text_);
        assert_equals(1,2);
    }

    void replaySomething()
    {
        engine().coutln(u"Replay doSomething: {}", text_);
    }
};



namespace {



auto Suite1 = register_class_suite<SampleSuite>("SampleSuite");


}


}}}
