
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

        size_t total_bytes = 1024 * 256 * 1024;
        size_t producer_block_size = 1024 * 128;
        size_t consumer_block_size = 1024 * 32;

        auto pipe = open_pipe();
        uint8_t generator_state{};
        uint8_t reader_state{};

        fibers::fiber producer([&]{
            auto buf = allocate_system_zeroed<uint8_t>(producer_block_size);

            size_t total{};

            while (total < total_bytes)
            {
                size_t max = (producer_block_size + total <= total_bytes) ? producer_block_size : (total_bytes - total);
                size_t size = max > 10 ? getNonZeroRandomG(max) : max;

                for (size_t c = 0; c < size; c++)
                {
                    *(buf.get() + c) = generator_state++;
                }

                total += pipe.output.write(buf.get(), size);
            }

            engine().coutln("Total written: {}", total);
        });


        fibers::fiber consumer([&]{
            auto buf = allocate_system_zeroed<uint8_t>(consumer_block_size);

            size_t total{};

            while (total < total_bytes)
            {
                size_t max = (consumer_block_size + total <= total_bytes) ? consumer_block_size : (total_bytes - total);
                size_t size = max > 10 ? getNonZeroRandomG(max) : max;

                size_t read = pipe.input.read(buf.get(), size);

                for (size_t c = 0; c < read; c++)
                {
                    uint8_t value = *(buf.get() + c);
                    if (value != reader_state) {
                        engine().coutln("Data checksum error at {}:{}:{}:{} {}:{}", total, c, read, size, (int)value, (int)reader_state);
                        std::terminate();
                    }

                    reader_state++;
                }

                total += read;
            }

            engine().coutln("Total read: {}", total);
        });

        consumer.join();
        producer.join();

        return 0;
    });
}


