
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

#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

using namespace memoria::v1;
using namespace memoria::v1::reactor;

namespace po = boost::program_options;


int main(int argc, char** argv, char** envp) {
    po::options_description options;

    options.add_options()
        ("host,h", po::value<std::string>()->default_value("0.0.0.0"), "Host address to bind to or 0.0.0.0")
        ("port,p", po::value<int32_t>()->default_value(5000), "Host port to bind to, default 5000")
        ("block,b", po::value<size_t>()->default_value(64), "Mean transfer block size, default 64 bytes")
        ("size,s", po::value<uint64_t>()->default_value(100 * 1024 * 1024), "Data size to transfer, in bytes. Default is 100M")
        ;

    return Application::run_e(options, argc, argv, envp, [&]{
        ShutdownOnScopeExit hh;

        Seed(getTimeInMillis());
        SeedBI(getTimeInMillis());

        std::string host    = app().options()["host"].as<std::string>();
        int32_t port        = app().options()["port"].as<int32_t>();
        size_t block_size   = app().options()["block"].as<size_t>();
        uint64_t data_size  = app().options()["size"].as<uint64_t>();

        engine().coutln(u"Host: {}, port: {}, mean block size: {}, data size: {}", host, port, block_size, data_size);

        ClientSocket socket(IPAddress(host.data()), port);

        engine().coutln(u"Connected!{}","");

        size_t block_size_max = block_size * 2;
        auto buffer = allocate_system<uint8_t>(block_size_max);
        uint8_t* buffer_ptr = buffer.get();

        auto looper_ostream = socket.output();
        auto looper_istream = socket.input();

        uint64_t sent_size_cnt{};
        uint64_t received_size_cnt{};

        uint64_t sent_blocks_cnt{};
        uint64_t received_blocks_cnt{};

        uint8_t stream_state{};

        int64_t start_time = getTimeInMillis();

        while (sent_size_cnt < data_size && received_size_cnt < data_size && !socket.is_closed())
        {
            if (sent_size_cnt < data_size)
            {
                size_t block = getRandomG(block_size_max - 1) + 1;

                for (size_t c = 0; c < block; c++) {
                    buffer_ptr[c] = stream_state++;
                }

                size_t written = looper_ostream.write(buffer_ptr, block);

                sent_size_cnt += written;
                sent_blocks_cnt++;
            }

            if (received_size_cnt < data_size)
            {
                size_t block = getRandomG(block_size_max - 1) + 1;

                received_size_cnt += looper_istream.read(buffer_ptr, block);
                received_blocks_cnt++;
            }

            //engine().coutln(u"Processed: {} {}", sent_size_cnt, sent_size_cnt);
        }

        socket.close();

        int64_t end_time = getTimeInMillis();
        int64_t duration = end_time - start_time;

        uint64_t sent_speed{};
        uint64_t recv_speed{};

        if (duration / 1000 > 0)
        {
            sent_speed = sent_blocks_cnt / (duration / 1000);
            recv_speed = received_blocks_cnt / (duration / 1000);
        }

        engine().cerrln(u"Time: {}s, blocks sent: {}, block recv: {}, speed: {} {} blocks/sec", FormatTime(duration), sent_blocks_cnt, received_blocks_cnt, sent_speed, recv_speed);


        return 0;
    });
}
