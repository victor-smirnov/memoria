 
// Copyright 2018-2023 Victor Smirnov
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


#include <memoria/memoria.hpp>
#include <memoria/tests/tests.hpp>
#include <memoria/tests/runner.hpp>

#include <memoria/seastar/seastar.hpp>

#include <seastar/core/app-template.hh>
#include <seastar/core/thread.hh>
#include <seastar/core/reactor.hh>

using namespace memoria;
using namespace seastar;

namespace po = boost::program_options;

int main(int argc, char** argv, char** envp)
{
    //InitMemoriaCoreExplicit();
    InitMemoriaExplicit();

    seastar::app_template::seastar_options opts;

    opts.smp_opts.memory_allocator = seastar::memory_allocator::standard;
    opts.smp_opts.smp.set_value(1);

    opts.smp_opts.thread_affinity.set_value(false);

    seastar::app_template app(std::move(opts));
    tests::set_current_app(&app);

    app.add_options()
        ("runs", "Number of runs for entire test suites set")
        ("test", po::value<std::string>(), "Specific test name to run")
        ("replay", "Run the test in replay mode, implies --test is specified")
        ("server", po::value<std::string>(), "Run as worker and specify the server address")
        ("port", po::value<uint16_t>(), "Run as worker and specify the server port")
        ("worker-num", po::value<size_t>(), "Worker's numeric identifier")
        ("workers", po::value<size_t>(), "Number of workers (default is number of cores/2)")
        ("print", "Print available test names")
        ("config", po::value<std::string>(), "Path to config file, defaults to tests2.yaml")
        ("output", po::value<std::string>(), "Path to tests' output directory, defaults to tests2.out in the CWD")
        ("coverage",
            po::value<std::string>()->default_value("tiny"),
            "Test coverage type: smoke, tiny, small, medium, large, xlarge"
        )
        ;

    return app.run(argc, argv, [&] {
        thread_attributes ta;
        ta.stack_size = 1024*1024;

        return seastar::async(ta, [&]() -> int {
            auto& config = app.configuration();

            std::string coverage = config["coverage"].as<std::string>();
            if (!tests::coverage_from_string(coverage))
            {
                println("Invalid test coverage type: {}", coverage);
                return 1;
            }

            if (config.count("test") > 0)
            {
                U8String test_name = U8String(config["test"].as<std::string>());

                bool replay = config.count("replay") > 0;

                if (replay) {
                    return (tests::replay_single_test(test_name) == tests::TestStatus::PASSED ? 0 : 1);
                }
                else {
                    return (tests::run_single_test(test_name) == tests::TestStatus::PASSED ? 0 : 1);
                }
            }
            else if (config.count("server"))
            {
                if (config.count("port") == 0) {
                    println("Server --port number must be specified");
                    return 1;
                }

                if (config.count("worker-num") == 0) {
                    println("Worker's number must be specified (--worker-num)");
                    return 1;
                }

                if (config.count("output") == 0) {
                    println("Worker's output folder must be specified (--output)");
                    return 1;
                }

                U8String address = config["server"].as<std::string>();
                uint16_t port = config["port"].as<uint16_t>();
                size_t worker_num = config["worker-num"].as<size_t>();

                println("Starting worker {}, server {}, port {}", worker_num, address, port);

                boost::filesystem::path output_folder(config["output"].as<std::string>());

                //tests::Worker worker(seastar::ipv4_addr(address, port), worker_num, output_folder);
                //worker.run();
                return 0;
            }
            else {
                size_t workers = std::thread::hardware_concurrency();

                if (workers == 0) {
                    workers = 1;
                }

                if (config.count("workers") != 0) {
                    workers = config["workers"].as<size_t>();
                }

                tests::run_tests2(workers);
                println("Done...");
            }

            return 0;
        });
    });
}
