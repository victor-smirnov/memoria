
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
        engine().println("HRPC Server");

        auto endpoints = hrpc::EndpointRepository::make();

        endpoints->add_handler(NORMAL_RQ_TEST, normal_rq_handler);


//        service->add_handler(ENDPOINT1, [](const PoolSharedPtr<hrpc::Context>& ctx){
//            println("Endpoint {} is called!", ctx->endpoint_id());
//            println("Request: {}", ctx->request().to_pretty_string());

//            auto i_channel = ctx->input_channel(0);
//            hrpc::Message msg;
//            while (i_channel->pop(msg)) {
//                println("Channel msg: {}", msg.to_pretty_string());
//            }

//            hrpc::Response rs = hrpc::Response::ok();
//            return rs;
//        });

        auto server_cfg = hrpc::TCPServerSocketConfig::of_host("0.0.0.0");
        auto server = hrpc::make_tcp_server(server_cfg, endpoints);

        fibers::fiber ff_server([&](){
            auto conn = server->new_session();

            conn->handle_messages();
            println("Server connection is done");
            conn->close();
        });


        server->listen();
        ff_server.join();

        return 0;
    }
    );
}
