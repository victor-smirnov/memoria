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
        engine().println("HRPC Client");

        auto service = hrpc::EndpointRepository::make();
        hrpc::EndpointID endpoint_id = hrpc::EndpointID::parse("{1|7d50cb1d2f4ec5ffcb7b22a423c6022873b1dc8420a1b01b264dc5b2b41f3d}");

        auto client_cfg = hrpc::TCPClientSocketConfig::of_host("127.0.0.1");
        auto conn = hrpc::make_tcp_client(client_cfg, service);

        fibers::fiber ff_conn_h([=](){
            conn->handle_messages();
            println("Client connection is done");
        });

        hrpc::Request rq = hrpc::Request::make();
        rq.set_output_channels(1);

        auto call = conn->call(endpoint_id, rq, [=](hrpc::Response rs){
            println("Response: {}", rs.to_pretty_string());
            conn->close();
        });

        this_fiber::yield();

        auto o_channel = call->output_channel(0);

        for (int c = 0; c < 3; c++) {
            auto batch = hrpc::Message::empty();
            o_channel->push(batch);
            this_fiber::yield();
        }

        o_channel->close();

        call->wait();

        ff_conn_h.join();

        return 0;
    });
}
