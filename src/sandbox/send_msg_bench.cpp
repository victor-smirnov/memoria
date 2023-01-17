#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/application.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/core/tools/time.hpp>

#include <memoria/core/hrpc/hrpc.hpp>
#include <memoria/core/hrpc/hrpc_async.hpp>

using namespace memoria;
using namespace memoria::reactor;

int main(int argc, char** argv, char** envp) {

    hrpc::Request rq;
    rq.set_call_id(0);

    boost::program_options::options_description dd;
    return Application::run_e(
        dd, argc, argv, envp,
        []() -> int32_t {
        ShutdownOnScopeExit hh;

        engine().println("Hello world!");

        int64_t t0 = memoria::getTimeInMillis();

        MessageQueue queue = MessageQueue::make();

        for (size_t c = 1; c < engine().cpu_num(); c++) {
            engine().add_queue_to(queue, c);
        }

        int64_t total = 1000000;
//        int cpu = engine().cpu();
        int tgt = 0;
        for (int64_t ii = 0; ii < total; ii++, tgt++) {
//            if (tgt == 15) {
//                tgt = 0;
//            }

//            engine().run_at(1 + tgt, [=]() {
//                return ii + 1;
//            });

            engine().run_async(queue, [=]() {
                return ii + 1;
            });

//            engine().run(queue, [=]() {
//                return engine().cpu();
//            });

            //engine().println("cpu = {}", cpu);

//            DummyMessage* msg = DummyMessage::make_instance(cpu);
//            engine().send_message(queue, msg);

            if (ii % 16 == 0) {
                memoria::this_fiber::yield();
            }
        }

        int64_t t1 = memoria::getTimeInMillis() + 1;

        engine().println("Execution time: {} ms, ops/s = {}", t1 - t0, total / (t1 - t0) * 1000);
        return 0;
    }
    );
}
