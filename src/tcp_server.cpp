
#include "memoria/v1/fiber/all.hpp"
#include "memoria/v1/reactor/application.hpp"
#include "memoria/v1/filesystem/path.hpp"
#include "memoria/v1/filesystem/operations.hpp"
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <iostream>
#include <thread>
#include <vector>

using namespace memoria::v1;
using namespace memoria::v1::reactor;



int main(int argc, char** argv) {
    return Application::run(argc, argv, [&]{
        ShutdownOnScopeExit hh;

        size_t total_bytes = 1024 * 1024 * 256;
        size_t producer_block_size = 1024 * 128;

        ServerSocket ss(IPAddress(127,0,0,1), 5556);
        ss.listen();

        uint8_t generator_state{};

        fibers::fiber producer([&]{

            ServerSocketConnection conn = ss.accept();
            auto out = conn.output();

            auto buf = allocate_system_zeroed<uint8_t>(producer_block_size);

            size_t total{};

            while (total < total_bytes)
            {
                size_t size = getRandomG(producer_block_size - 100) + 100;

                for (size_t c = 0; c < size; c++) *(buf.get() + c) = generator_state++;

                //std::cout << "To write: " << size << std::endl;
                total += out.write(buf.get(), size);
                std::cout << "To write: " << size << " " << total << std::endl;
            }

            engine().coutln(u"Total written: {}", total);
        });
        producer.join();

        return 0;
    });
}


