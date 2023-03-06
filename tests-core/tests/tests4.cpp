
// Copyright 2023 Victor Smirnov
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

#include <memoria/core/strings/format.hpp>

#include <boost/fiber/all.hpp>
#include <memoria/asio/round_robin.hpp>
#include <memoria/asio/yield.hpp>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

using namespace memoria;

int main(int argc, char** argv) {
    try {

        po::options_description description("Usage:");

        description.add_options()
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

        po::variables_map& config = tests::get_config();
        po::store(po::command_line_parser(argc, argv).options(description).run(), config);
        po::notify(config);

        std::shared_ptr< boost::asio::io_service > io_svc = std::make_shared< boost::asio::io_service >();
        boost::fibers::use_scheduling_algorithm< memoria::asio::round_robin >( io_svc);

        auto res = boost::fibers::async([&]{
            println("In a fiber!");
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

//                tests::Worker worker(address, port, worker_num, output_folder);
//                worker.run();
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

                tests::run_tests2(*io_svc, workers);
                println("Done...");
                io_svc->stop();
                return 11;
            }

        });

        io_svc->run();

        return res.get();
    }
    catch (const std::exception& e) {
        println("Exception: ", e.what(), "\n");
    }

    return EXIT_FAILURE;
}
