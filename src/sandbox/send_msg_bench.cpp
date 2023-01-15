#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/application.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/core/tools/time.hpp>

using namespace memoria::reactor;

int main(int argc, char** argv, char** envp) {

    boost::program_options::options_description dd;
    return Application::run_e(
        dd, argc, argv, envp,
        []() -> int32_t {
        ShutdownOnScopeExit hh;

        engine().println("Hello world!");

        int64_t t0 = memoria::getTimeInMillis();

        int64_t total = 10000000;
        int cpu = engine().cpu();
        for (int64_t ii = 0; ii < total; ii++) {
//            engine().run_at_async(1, [=]() {
//                return ii + 1;
//            });

            DummyMessage* msg = DummyMessage::make_instance(cpu);
            engine().send_message(1, msg);

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
