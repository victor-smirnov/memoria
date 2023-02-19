
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


#include <memoria/core/hrpc/hrpc.hpp>

#include <memoria/memoria.hpp>

#include "hrpc_tests_common.hpp"

#include <seastar/core/app-template.hh>
#include <seastar/core/thread.hh>

using namespace memoria;
namespace ss = seastar;

int main(int argc, char** argv, char** envp)
{


    ss::app_template::seastar_options opts;

    opts.smp_opts.memory_allocator = ss::memory_allocator::standard;
    opts.smp_opts.smp.set_value(1);
    opts.smp_opts.thread_affinity.set_value(false);

    ss::app_template app(std::move(opts));

    int code = app.run(
        argc, argv,
        []()
    {
        InitMemoriaExplicit();

        return ss::async([]{
            println("HRPC Client");

            auto endpoints = hrpc::EndpointRepository::make();

            auto client_cfg = hrpc::TCPClientSocketConfig::of_host("127.0.0.1");
            auto session = hrpc::open_tcp_session(client_cfg, endpoints);

            seastar::thread ff_conn_h([=](){
                session->handle_messages();
                println("Client connection is done");
            });

            normal_rq_cient(session);

            session->close();
            ff_conn_h.join().get();

            println("Client is done");

            return 0;
        });
    });

    println("Seastar is done");

    return code;
}
