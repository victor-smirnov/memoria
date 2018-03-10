
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
#include <memoria/v1/reactor/socket.hpp>

#include "stream_test.hpp"

namespace memoria {
namespace v1 {
namespace tests {

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


auto sokcet_test = register_test_in_suite<FnTest<SocketTestState>>(u"ReactorSuite", u"SocketTest", [](auto& state){

    int32_t outbound_port = getRandomG(1000);
    int32_t inbound_port  = getRandomG(1000);

    reactor::ServerSocket sender_server_socket(reactor::IPAddress(127,0,0,1), 5000 + outbound_port);
    sender_server_socket.listen();

    reactor::ServerSocket looper_server_socket(reactor::IPAddress(127,0,0,1), 5000 + inbound_port);
    looper_server_socket.listen();

    reactor::ServerSocketConnection sender_server_connection;
    reactor::ServerSocketConnection looper_server_connection;


    reactor::ClientSocket looper_client_connection;
    reactor::ClientSocket receiver_client_connection;

    fibers::fiber sender_connector([&]{
        sender_server_connection = sender_server_socket.accept();
    });

    fibers::fiber looper_connector([&]{
        looper_client_connection = reactor::ClientSocket(reactor::IPAddress(127,0,0,1), 5000 + outbound_port);
        looper_server_connection = looper_server_socket.accept();
    });

    fibers::fiber receiver_connector([&]{
        receiver_client_connection = reactor::ClientSocket(reactor::IPAddress(127,0,0,1), 5000 + inbound_port);
    });



    sender_connector.join();
    looper_connector.join();
    receiver_connector.join();

    TestRunner runner(
            sender_server_connection.output(),

            looper_client_connection.input(),
            looper_server_connection.output(),

            receiver_client_connection.input(),

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
