
// Copyright 2023 Victor Smirnov
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


#include <memoria/hrpc/hrpc.hpp>

#include <memoria/memoria_core.hpp>

#include "hrpc_tests_common.hpp"

#include <memoria/asio/hrpc/hrpc.hpp>

#include <memoria/asio/reactor.hpp>
#include <memoria/asio/round_robin.hpp>

#include <boost/fiber/all.hpp>

#include <catch2/catch_session.hpp>

using namespace memoria;
using namespace memoria::asio;


int main(int argc, char** argv, char** envp)
{
    InitMemoriaCoreExplicit();

    IOContextPtr io_ctx = std::make_shared<IOContext>();
    set_io_context(io_ctx);

    boost::fibers::use_scheduling_algorithm< memoria::asio::round_robin>(io_ctx);

    int result;
    boost::fibers::fiber([&]{
        println("HRPC Client running Catch2 tests");

        auto endpoints = memoria::hrpc::st::EndpointRepository::make();

        auto client_cfg = memoria::hrpc::TCPClientSocketConfig::of_host("127.0.0.1");
        auto session = memoria::asio::hrpc::open_tcp_session(client_cfg, endpoints);

        set_session(session);
        auto dtr = MakeOnScopeExit([]{
            set_session(SessionPtr{});
        });

        boost::fibers::fiber ff_conn_h([=](){
            session->handle_messages();
            println("Client connection is done");
        });

        const char* argv0[] = {"test", 0};
        result = Catch::Session().run( 1, argv0 );

        session->close();
        ff_conn_h.join();

        println("Session has been closed");
        println("Client is done");

        io_context().stop();
    }).detach();

    io_ctx->run();

    return result;

}
