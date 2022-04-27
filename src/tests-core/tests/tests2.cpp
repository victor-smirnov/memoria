 
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

#include <memoria/memoria.hpp>

#include <memoria/reactor/application.hpp>

#include <memoria/tests/runner.hpp>
#include <memoria/tests/arg_helper.hpp>



using namespace memoria;
using namespace memoria::reactor;

namespace po = boost::program_options;

int main(int argc, char** argv, char** envp)
{
    //InitMemoriaCoreExplicit();
    InitMemoriaExplicit();

    tests::ThreadsArgHelper helper(argv);
    po::options_description options;

    options.add_options()
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

    return Application::run_e(options, helper.argc(), helper.args(), envp, [&]{
        ShutdownOnScopeExit hh;

        std::string coverage = app().options()["coverage"].as<std::string>();
        if (!tests::coverage_from_string(coverage))
        {
            engine().println("Invalid test coverage type: {}", coverage);
            return 1;
        }

        if (app().options().count("print") > 0)
        {
            tests::tests_registry().print();
            return 0;
        }

        if (app().options().count("test") > 0)
        {
            U8String test_name = U8String(app().options()["test"].as<std::string>());

            bool replay = app().options().count("replay") > 0;

            if (replay) {
                return (tests::replay_single_test(test_name) == tests::TestStatus::PASSED ? 0 : 1);
            }
            else {
                return (tests::run_single_test(test_name) == tests::TestStatus::PASSED ? 0 : 1);
            }
        }
        else if (app().options().count("server"))
        {
            if (app().options().count("port") == 0) {
                engine().println("Server --port number must be specified");
                return 1;
            }

            if (app().options().count("worker-num") == 0) {
                engine().println("Worker's number must be specified (--worker-num)");
                return 1;
            }

            if (app().options().count("output") == 0) {
                engine().println("Worker's output folder must be specified (--output)");
                return 1;
            }

            IPAddress address = parse_ipv4(app().options()["server"].as<std::string>());
            uint16_t port = app().options()["port"].as<uint16_t>();
            size_t worker_num = app().options()["worker-num"].as<size_t>();

            println("Starting worker {}, server {}, port {}", worker_num, address.to_string(), port);

            filesystem::path output_folder(app().options()["output"].as<std::string>());

            tests::Worker worker(address, port, worker_num, output_folder);
            worker.run();
            return 0;
        }
        else {
            size_t workers = std::thread::hardware_concurrency();

            if (workers == 0) {
                workers = 1;
            }

            if (app().options().count("workers") != 0) {
                workers = app().options()["workers"].as<size_t>();
            }

            tests::run_tests2(workers);

            engine().println("Done...");
        }

        return 0;
    });
}
