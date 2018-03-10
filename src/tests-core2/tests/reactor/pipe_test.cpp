
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
#include <memoria/v1/reactor/pipe_streams.hpp>

#include "stream_test.hpp"

namespace memoria {
namespace v1 {
namespace tests {

using TestRunner = StreamTester<reactor::PipeInputStream, reactor::PipeOutputStream>;

struct PipeTestState: TestState {
    using Base = TestState;

    TestDuration duration;

    virtual void post_configure(TestCoverage coverage)
    {
        if (coverage == TestCoverage::SMOKE) {
            duration = std::chrono::milliseconds(100);
        }
        else if (coverage == TestCoverage::TINY) {
            duration = std::chrono::milliseconds(500);
        }
        else if (coverage == TestCoverage::SMALL) {
            duration = std::chrono::seconds(1);
        }
        else if (coverage == TestCoverage::MEDIUM) {
            duration = std::chrono::seconds(10);
        }
        else if (coverage == TestCoverage::LARGE) {
            duration = std::chrono::seconds(60);
        }
        else {
            duration = std::chrono::seconds(300);
        }
    }
};




auto test = register_test_in_suite<FnTest<PipeTestState>>(u"ReactorSuite", u"PipeTest", [](auto& state){
    auto outbound_pipe = reactor::open_pipe();
    auto inbound_pipe  = reactor::open_pipe();

    TestRunner runner(
            outbound_pipe.input,
            outbound_pipe.output,
            inbound_pipe.input,
            inbound_pipe.output,
            state.duration,
            1,
            65536
    );

    runner.start();
    runner.join();

	reactor::engine().coutln(u"Sent {} bytes", runner.total_sent());
	reactor::engine().coutln(u"Transferred {} bytes", runner.total_transferred());
	reactor::engine().coutln(u"Received {} bytes", runner.total_received());

    assert_equals(runner.total_sent(), runner.total_received(), "Sent/received");
    assert_equals(runner.total_sent(), runner.total_transferred(), "Sent/transferred");
});

}}}
