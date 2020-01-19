
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

#include <memoria/tests/tests.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/socket.hpp>

#include <memoria/core/tools/time.hpp>

#include "stream_test.hpp"

namespace memoria {
namespace tests {

using namespace memoria::reactor;

using TestRunner = StreamTester<BinaryInputStream, BinaryOutputStream>;

struct SocketTestState: TestState {
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


auto sokcet_test = register_test_in_suite<FnTest<SocketTestState>>("ReactorSuite", "SocketTest", [](auto& state){

    int32_t outbound_port = getRandomG(1000);
    int32_t inbound_port  = getRandomG(1000);

    int32_t port_base = 11000;

    ServerSocket sender_server_socket(IPAddress(127,0,0,1), port_base + outbound_port);
    sender_server_socket.listen();

    ServerSocket looper_server_socket(IPAddress(127,0,0,1), port_base + inbound_port);
    looper_server_socket.listen();

    TestRunner runner(
            [&]{
                ServerSocketConnection conn = sender_server_socket.accept();
                return conn.output();
            },

            [&]{
                return reactor::ClientSocket(IPAddress(127,0,0,1), port_base + outbound_port).input();
            },

            [&]{
                ServerSocketConnection conn = looper_server_socket.accept();
                return conn.output();
            },

            [&]{
                return reactor::ClientSocket(IPAddress(127,0,0,1), port_base + inbound_port).input();
            },

            state.duration,
            1,
            64
    );

    runner.start();
    runner.join();

    reactor::engine().coutln("Sent {} bytes", runner.total_sent());
    reactor::engine().coutln("Transferred {} bytes", runner.total_transferred());
    reactor::engine().coutln("Received {} bytes", runner.total_received());

    assert_equals(runner.total_sent(), runner.total_received(), "Sent/received");
    assert_equals(runner.total_sent(), runner.total_transferred(), "Sent/transferred");
});



}}
