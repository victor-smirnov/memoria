
#include <memoria/v1/fiber/all.hpp>
#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/filesystem/path.hpp>
#include <memoria/v1/filesystem/operations.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/reactor/pipe_streams.hpp>

#include <iostream>
#include <thread>
#include <vector>

#include <fcntl.h>

using namespace memoria::v1;
using namespace memoria::v1::reactor;


int main(int argc, char** argv) {
    return Application::run(argc, argv, [&]{
        ShutdownOnScopeExit hh;

        size_t total_bytes = 1024 * 256;
        size_t producer_block_size = 1024 * 128;
        size_t consumer_block_size = 1024 * 32;

        ServerSocket ss(IPAddress(127,0,0,1), 5556);
        ss.listen();


        //auto pipe = open_pipe();

        fibers::fiber producer([&]{

            ServerSocketConnection conn = ss.accept();
            auto out = conn.output();

            auto buf = allocate_system_zeroed<uint8_t>(producer_block_size);

            size_t total{};

            while (total < total_bytes)
            {
                size_t size = getRandomG(producer_block_size - 100) + 100;
                std::cout << "To write: " << size << std::endl;
                total += out.write(buf.get(), size);
            }

            engine().coutln(u"Total written: {}", total);
        });


        fibers::fiber consumer([&]{

            ClientSocket cs(IPAddress(127,0,0,1), 5556);

            auto input = cs.input();

            auto buf = allocate_system_zeroed<uint8_t>(consumer_block_size);

            size_t total{};

            while (total < total_bytes)
            {
                size_t size = getRandomG(consumer_block_size - 100) + 100;
                std::cout << "To read: " << size << std::endl;
                total += input.read(buf.get(), size);
            }

            engine().coutln(u"Total read: {}", total);
        });

        consumer.join();
        producer.join();

        return 0;
    });
}


