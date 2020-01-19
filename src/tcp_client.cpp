
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


#include "memoria/fiber/all.hpp"
#include "memoria/reactor/application.hpp"
#include "memoria/filesystem/path.hpp"
#include "memoria/filesystem/operations.hpp"
#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/time.hpp>

#include <iostream>
#include <thread>
#include <vector>

using namespace memoria::v1;
using namespace memoria::reactor;

int main(int argc, char** argv) {
    return Application::run(argc, argv, [&]{
        ShutdownOnScopeExit hh;

		Seed(getTimeInMillis() + (size_t) argv);

        size_t total_bytes = 1024 * 1024 * 256;
        size_t consumer_block_size = 1024 * 32;

        uint8_t reader_state{};

        ClientSocket cs(IPAddress(127,0,0,1), 5556);

        auto input = cs.input();

        auto buf = allocate_system_zeroed<uint8_t>(consumer_block_size);

        size_t total{};

        while (total < total_bytes)
        {
			size_t max = (consumer_block_size + total <= total_bytes) ? consumer_block_size : (total_bytes - total);
			size_t size = max > 1 ? getNonZeroRandomG(max) : max;
			
			size_t read = input.read(buf.get(), size);
			
			
            for (size_t c = 0; c < read; c++)
            {
                uint8_t value = *(buf.get() + c);
                if (value != reader_state)
                {
                    engine().coutln("Data checksum error at {}:{}:{}:{} {}:{}", total, c, read, size, (int)value, (int)reader_state);
                    std::terminate();
                }
                ++reader_state;
            }

			total += read;
        }



        engine().coutln("Total read: {}", total);
        

        

        return 0;
    });
}
