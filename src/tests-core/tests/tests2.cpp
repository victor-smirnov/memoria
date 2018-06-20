
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

#include <memoria/v1/tests/runner.hpp>
#include <memoria/v1/tests/arg_helper.hpp>

using namespace memoria::v1;
using namespace memoria::v1::reactor;

namespace po = boost::program_options;

int main(int argc, char** argv, char** envp)
{
    tests::ThreadsArgHelper helper(argv);

    po::options_description options;

    options.add_options()
        ("runs", "Number of runs for entire test suites set")
        ("test", po::value<std::string>(), "Specific test name to run")
        ("replay", "Run the test in replay mode, implies --test is specified")
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
            engine().coutln(u"Invalid test coverage type: {}", coverage);
            return 1;
        }

        if (app().options().count("test") > 0)
        {
            U16String test_name = U16String(app().options()["test"].as<std::string>());

            bool replay = app().options().count("replay") > 0;

            if (replay) {
                return (tests::replay_single_test(test_name) == tests::TestStatus::PASSED ? 0 : 1);
            }
            else {
                return (tests::run_single_test(test_name) == tests::TestStatus::PASSED ? 0 : 1);
            }
        }
        else {
            tests::run_tests();
        }

        return 0;
    });
}
