
// Copyright 2017 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


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
        size_t consumer_block_size = 1024 * 32;

        uint8_t reader_state{};

        fibers::fiber consumer([&]{

            ClientSocket cs(IPAddress(127,0,0,1), 5556);

            auto input = cs.input();

            auto buf = allocate_system_zeroed<uint8_t>(consumer_block_size);

            size_t total{};
            size_t errors{};

            while (total < total_bytes)
            {
                size_t base = total;
                size_t size = getRandomG(consumer_block_size - 100) + 100;
                total += input.read(buf.get(), size);
                std::cout << "Read: " << size << " " << total << std::endl;

                for (size_t c = 0; c < size; c++)
                {
                    uint8_t value = *(buf.get() + c);
                    if (value != reader_state)
                    {
                        errors++;
                        engine().coutln(u"Data checksum error at {}: {}:{}", base + c, (int)value, (int)reader_state);

                        if (errors >= 10) {
                            std::terminate();
                        }
                    }
                    ++reader_state;
                }
            }

            engine().coutln(u"Total read: {}", total);
        });

        consumer.join();

        return 0;
    });
}
