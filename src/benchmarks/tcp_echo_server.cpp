
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
        ;

    return Application::run_e(options, argc, argv, envp, [&]{
        ShutdownOnScopeExit hh;

        Seed(getTimeInMillis());
        SeedBI(getTimeInMillis());

        std::string host  = app().options()["host"].as<std::string>();
        int32_t port      = app().options()["port"].as<int32_t>();
        size_t block_size = app().options()["block"].as<size_t>();

        engine().coutln(u"Host: {}, port: {}, block size: {}", host, port, block_size);

        ServerSocket socket(IPAddress(host.data()), port);
        socket.listen();

        std::vector<fibers::fiber> active_fibers;

        while (true)
        {
            ServerSocketConnection csconn = socket.accept();

            active_fibers.emplace_back(fibers::fiber([&](auto conn){

                int64_t start_time = getTimeInMillis();

                size_t block_size_max = block_size * 2;
                auto buffer = allocate_system<uint8_t>(block_size_max);
                uint8_t* buffer_ptr = buffer.get();

                auto looper_ostream = conn.output();
                auto looper_istream = conn.input();

                uint64_t cnt{};

                while (!conn.is_closed())
                {
                    size_t block = getRandomG(block_size_max - 1) + 1;
                    size_t read = looper_istream.read(buffer_ptr, block);
                    looper_ostream.write(buffer_ptr, read);
                    cnt++;
                }

                int64_t end_time = getTimeInMillis();
                int64_t duration = end_time - start_time;

                uint64_t speed{};
                if (duration / 1000 > 0) {
                    speed = cnt / (duration / 1000);
                }

                engine().cerrln(u"Time: {}, blocks: {}, speed: {} blocks/sec", FormatTime(duration), cnt, speed);
            }, csconn));
        }

        socket.close();
        return 0;
    });
}
