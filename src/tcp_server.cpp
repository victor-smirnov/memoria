
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

		Seed(getTimeInMillis());

        ServerSocket ss(IPAddress(127,0,0,1), 5556);
        ss.listen();

        uint8_t generator_state{};

        ServerSocketConnection conn = ss.accept();

		engine().coutln("Client connected", 1);

        auto out = conn.output();

        auto buf = allocate_system_zeroed<uint8_t>(producer_block_size);

        size_t total{};

        while (total < total_bytes)
        {
			size_t max = (producer_block_size + total <= total_bytes) ? producer_block_size : (total_bytes - total);
            size_t size = max > 10 ? getNonZeroRandomG(max) : max;

			for (size_t c = 0; c < size; c++) {
				*(buf.get() + c) = generator_state++;
			}

            size_t written = out.write(buf.get(), size);

			total += written;
        }

        engine().coutln("Total written: {}", total);
        

        return 0;
    });
}


