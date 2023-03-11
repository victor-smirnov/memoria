
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


#include <memoria/core/tools/time.hpp>

#include <memoria/hrpc/hrpc.hpp>
#include <memoria/hrpc/hrpc_async.hpp>

#include <memoria/hrpc/hrpc.hpp>

#include <memoria/memoria_core.hpp>

#include "hrpc_tests_common.hpp"

#include <memoria/asio/hrpc/hrpc.hpp>
#include <memoria/asio/reactor.hpp>
#include <memoria/asio/round_robin.hpp>

#include <boost/fiber/all.hpp>

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
        println("HRPC Server");

        auto endpoints = memoria::hrpc::st::EndpointRepository::make();

        endpoints->add_handler(NORMAL_RQ_TEST, normal_rq_handler);
        endpoints->add_handler(ERRORS_TEST, errors_handler);
        endpoints->add_handler(INPUT_CHANNEL_TEST, input_stream_handler);
        endpoints->add_handler(OUTPUT_CHANNEL_TEST, output_stream_handler);
        endpoints->add_handler(CANCEL_RQ_TEST, cancel_rq_handler);

        auto server_cfg = memoria::hrpc::TCPServerSocketConfig::of_host("0.0.0.0");
        auto server = memoria::asio::hrpc::make_tcp_server(server_cfg, endpoints);

        boost::fibers::fiber ff_server([&](){
            auto conn = server->new_session();
            println("New server connection");

            conn->handle_messages();
            println("Server connection is done");
            conn->close();
        });


        server->listen();
        ff_server.join();

        io_context().stop();

        result = 0;
    }).detach();

    io_ctx->run();

    return result;
}
