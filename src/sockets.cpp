
#include <memoria/fiber/all.hpp>
#include <memoria/reactor/application.hpp>
#include <memoria/filesystem/path.hpp>
#include <memoria/filesystem/operations.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <memoria/core/memory/malloc.hpp>

#include <memoria/reactor/pipe_streams.hpp>

#include <iostream>
#include <thread>
#include <vector>

#include <fcntl.h>

using namespace memoria::v1;
using namespace memoria::reactor;


int main(int argc, char** argv) {
    return Application::run(argc, argv, [&]{
        ShutdownOnScopeExit hh;

        size_t total_bytes = 1024 * 1024 * 10;
        size_t producer_block_size = 1024 * 32;
        size_t consumer_block_size = 1024 * 32;

		Seed(getTimeInMillis());

        size_t port = 5000 + getRandomG(1000);

        ServerSocket ss(IPAddress(127,0,0,1), port);

		ss.listen();

		uint8_t generator_state{};
		uint8_t reader_state{};

        BinaryOutputStream out(nullptr);

        fibers::fiber producer([&]{

            ServerSocketConnection conn = ss.accept();
            out = conn.output();

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

                size_t written = out.write(buf.get(), size);
				total += written;

                std::cout << "Written " << written << std::endl;

                this_fiber::yield();
            }

            engine().coutln("Total written: {}", total);
        });


        fibers::fiber consumer([&]{

            ClientSocket cs(IPAddress(127,0,0,1), port);
            auto input = cs.input();

            auto buf = allocate_system_zeroed<uint8_t>(consumer_block_size);

			size_t total{};

            while (total < total_bytes)
            {
				size_t max = (consumer_block_size + total <= total_bytes) ? consumer_block_size : (total_bytes - total);
				size_t size = max > 10 ? getNonZeroRandomG(max) : max;

				size_t read = input.read(buf.get(), size);

                std::cout << "Read " << read << std::endl;

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

                this_fiber::yield();
            }

            engine().coutln("Total read: {}", total);
        });

        consumer.join();
        producer.join();

        return 0;
    });
}


