#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/application.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/core/tools/time.hpp>

#include <memoria/core/hrpc/hrpc.hpp>
#include <memoria/core/hrpc/hrpc_async.hpp>

#include <memoria/core/hrpc/hrpc.hpp>

#include <memoria/memoria.hpp>

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
        engine().println("HRPC Client/Server");

        auto service = hrpc::Service::make();
        hrpc::EndpointID endpoint_id = hrpc::EndpointID::parse("{1|7d50cb1d2f4ec5ffcb7b22a423c6022873b1dc8420a1b01b264dc5b2b41f3d}");

        service->add_handler(endpoint_id, [](const PoolSharedPtr<hrpc::Context>& ctx ){
            println("Endpoint {} is called!", ctx->endpoint_id());
            println("Request: {}", ctx->request().to_pretty_string());

            auto i_channel = ctx->input_channel(0);
            hrpc::Message msg;
            while (i_channel->pop(msg)) {
                println("Channel msg: {}", msg.to_pretty_string());
            }

            hrpc::Response rs = hrpc::Response::ok();            
            return rs;
        });

        auto server_cfg = hrpc::TCPServerSocketConfig::of_host("0.0.0.0");
        auto server = hrpc::make_tcp_server_socket(server_cfg, service);

        fibers::fiber ff_server([&](){
            auto conn = server->accept();

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
