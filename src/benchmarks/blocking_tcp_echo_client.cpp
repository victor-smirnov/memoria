
// Copyright 2018 Victor Smirnov
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


#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/memory/malloc.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using namespace memoria;

enum { mean_length = 4096 };
enum { data_size = 1000000000 };

int main(int argc, char* argv[])
{
    try
    {
//        if (argc != 3)
//        {
//            std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
//            return 1;
//        }

        boost::asio::io_context io_context;

        tcp::socket s(io_context);
        tcp::resolver resolver(io_context);
        //boost::asio::connect(s, resolver.resolve(argv[1], argv[2]));
        boost::asio::connect(s, resolver.resolve("localhost", "5000"));

        std::cout << "Connected!" << std::endl;

        size_t block_size_max = mean_length * 2;
        auto buffer = allocate_system<char>(block_size_max);
        char* buffer_ptr = buffer.get();

        uint64_t sent_size_cnt{};
        uint64_t received_size_cnt{};

        uint64_t sent_data_size{data_size};
        uint64_t received_data_size{data_size + 1};


        uint64_t sent_blocks_cnt{};
        uint64_t received_blocks_cnt{};

        char stream_state{};

        int64_t start_time = getTimeInMillis();

        while (sent_size_cnt < sent_data_size && received_size_cnt < received_data_size && s.is_open())
        {
            size_t block = mean_length;//getRandomG(block_size_max - 1) + 1;

            if (sent_size_cnt < sent_data_size)
            {
                for (size_t c = 0; c < block; c++) {
                    buffer_ptr[c] = stream_state++;
                }

                size_t written = boost::asio::write(s, boost::asio::buffer(buffer_ptr, block));

                sent_size_cnt += written;
                sent_blocks_cnt++;
            }

            if (received_size_cnt < received_data_size)
            {
                //size_t block = getRandomG(block_size_max - 1) + 1;

                received_size_cnt += boost::asio::read(s, boost::asio::buffer(buffer_ptr, block));
                received_blocks_cnt++;
            }
        }

        s.close();

        int64_t end_time = getTimeInMillis();
        double duration = (end_time - start_time) / 1000.0l;

        double sent_speed = sent_blocks_cnt / duration;
        double recv_speed = received_blocks_cnt / duration;

        std::cout << fmt::format("Time: {}s, blocks sent: {}, block recv: {}, speed: {} {} blocks/sec", FormatTime(duration * 1000), sent_blocks_cnt, received_blocks_cnt, sent_speed, recv_speed) << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
