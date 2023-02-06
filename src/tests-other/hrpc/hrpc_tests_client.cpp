
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


#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/application.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/core/tools/time.hpp>

#include <memoria/core/hrpc/hrpc.hpp>
#include <memoria/core/hrpc/hrpc_async.hpp>

#include <memoria/core/hrpc/hrpc.hpp>

#include <memoria/memoria.hpp>

#include "hrpc_tests_common.hpp"

using namespace memoria;
using namespace memoria::reactor;

int main(int argc, char** argv, char** envp) {

    InitMemoriaExplicit();

    boost::program_options::options_description dd;
    return Application::run_e(
        dd, argc, argv, envp,
        []() -> int32_t
    {
        ShutdownOnScopeExit hh;
        engine().println("HRPC Client");

        auto endpoints = hrpc::EndpointRepository::make();

        auto client_cfg = hrpc::TCPClientSocketConfig::of_host("127.0.0.1");
        auto session = hrpc::open_tcp_session(client_cfg, endpoints);

        fibers::fiber ff_conn_h([=](){
            session->handle_messages();
            println("Client connection is done");
        });

        normal_rq_cient(session);

//        hrpc::Request rq = hrpc::Request::make();
//        rq.set_output_channels(1);

//        auto call = conn->call(ENDPOINT1, rq, [=](hrpc::Response rs){
//            println("Response: {}", rs.to_pretty_string());
//            conn->close();
//        });

//        this_fiber::yield();

//        auto o_channel = call->output_channel(0);

//        for (int c = 0; c < 3; c++) {
//            auto batch = hrpc::Message::empty();
//            o_channel->push(batch);
//            this_fiber::yield();
//        }

//        o_channel->close();

//        call->wait();

        session->close();

        ff_conn_h.join();

        return 0;
    });
}
