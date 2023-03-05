
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

#include <memoria/reactor/hrpc/hrpc.hpp>

#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/application.hpp>


#include <catch2/catch_session.hpp>

using namespace memoria;
using namespace memoria::reactor;


int main(int argc, char** argv, char** envp)
{
    InitMemoriaCoreExplicit();

    return Application::run(
        argc, argv,
        [&]() -> int
    {
        ShutdownOnScopeExit hh;

        int result;

        in_fiber([&]{
            println("HRPC Client running Catch2 tests");

            auto endpoints = memoria::hrpc::st::EndpointRepository::make();

            auto client_cfg = memoria::hrpc::TCPClientSocketConfig::of_host("127.0.0.1");
            auto session = memoria::reactor::hrpc::open_tcp_session(client_cfg, endpoints);

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
            println("Session has been closed");
            ff_conn_h.join();

            println("Client is done");
        }).join();

        return result;
    });
}
