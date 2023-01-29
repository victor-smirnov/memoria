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

        auto service = hrpc::HRPCService::make();


        hrpc::EndpointID endpoint = 1;

        service->add_handler(endpoint, [](const PoolSharedPtr<hrpc::Context>& ctx ){
            ctx->connection()->close();
            println("Endpoint {} is called!", ctx->request().endpoint());
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
        auto client_cfg = hrpc::TCPClientSocketConfig::of_host("localhost");

        auto server = hrpc::make_tcp_server_socket(server_cfg, service);

        fibers::fiber ff_server([&](){
            auto conn = server->accept();

            conn->handle_messages();

            conn->close();
        });

        fibers::fiber ff_client([&](){
            auto client = hrpc::make_tcp_client_socket(client_cfg, service);

            auto conn = client->open();

            fibers::fiber ff_conn_h([=](){
                conn->handle_messages();
            });

            hrpc::Request rq = hrpc::Request::make();
            rq.set_endpoint(endpoint);
            rq.set_output_channels(1);

            auto call = conn->call(rq, [=](hrpc::Response rs){
                println("Response: {}", rs.to_pretty_string());
                conn->close();
            });

            auto o_channel = call->output_channel(0);

            for (int c = 0; c < 3; c++) {
                auto batch = hrpc::Message::empty();
                o_channel->push(batch);
            }

            o_channel->close();

            call->wait();

            ff_conn_h.join();
        });


        server->listen();

        ff_server.join();
        ff_client.join();

        return 0;
    }
    );
}
