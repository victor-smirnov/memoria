
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

#include <memoria/v1/tests/runner.hpp>
#include <memoria/v1/reactor/process.hpp>
#include <memoria/v1/reactor/pipe_streams_reader.hpp>
#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/reactor/file_streams.hpp>
#include <memoria/v1/filesystem/operations.hpp>

#include <memoria/v1/yaml-cpp/yaml.h>

#include <sstream>

namespace memoria {
namespace v1 {
namespace tests {

using filesystem::path;
using namespace reactor;

void dump_exception(std::ostream& out, std::exception_ptr& ex)
{
    try {
        std::rethrow_exception(ex);
    }
    catch (MemoriaThrowable& th) {
        th.dump(out);
    }
    catch (std::exception& th) {
        out << "STD Exception: " << th.what() << std::endl;
    }
    catch (...) {
        out << "Unknown exception" << std::endl;
    }
}

namespace {

filesystem::path get_tests_config_path()
{
    auto& app = reactor::app();

    filesystem::path config_path;

    if (app.options().count("config") > 0)
    {
        config_path = U16String(app.options()["config"].as<std::string>());
    }
    else {
        auto program_path = reactor::get_program_path();
        config_path = program_path.parent_path();
        config_path.append((get_image_name().to_u8() + ".yaml").data());
    }

    return config_path;
}

}

TestStatus run_single_test(const U16String& test_path)
{
    auto test = tests_registry().find_test(test_path);
    if (test)
    {
        filesystem::path config_path = get_tests_config_path();

        YAML::Node test_config;

        if (filesystem::is_regular_file(config_path))
        {

            YAML::Node config = YAML::LoadFile(config_path.to_u8().to_std_string());

            U16String suite_name;
            U16String test_name;

            std::tie(suite_name, test_name) = TestsRegistry::split_path(test_path);

            YAML::Node suite_node = config[suite_name.to_u8().to_std_string()];
            if (suite_node)
            {
                test_config = suite_node[test_name.to_u8().to_std_string()];
            }
        }

        DefaultTestContext ctx(test_config, config_path.parent_path());
        test.get().run(&ctx);

        if (ctx.status() == TestStatus::PASSED)
        {
            ctx.out() << "PASSED" << std::endl;
            reactor::engine().cout() << "PASSED" << std::flush;
        }
        else {
            ctx.out() << "FAILED" << std::endl;
            reactor::engine().cout() << "FAILED" << std::flush;
            if (ctx.ex())
            {
                dump_exception(ctx.out(), ctx.ex());
            }
        }

        return ctx.status();
    }
    else {
        MMA1_THROW(TestConfigurationException()) << fmt::format_ex(u"No test with path '{}' found", test_path);
    }
}

template <typename T>
U16String to_string(const std::vector<T>& array)
{
    U16String data = u"[";

    bool first = true;

    for (const auto& ee : array)
    {
        if (!first) {
            data += u", ";
        }
        else {
            first = false;
        }

        std::stringstream ss;
        ss << ee;

        data += U16String(ss.str());
    }

    data += u"]";
    return data;
}

namespace  {

enum class EnablementType {DISABLED, ENABLED, DEFAULT};

EnablementType is_enabled(YAML::Node& node)
{
    if (node["enable"]) {
        return node["enable"].as<bool>() ? EnablementType::ENABLED : EnablementType::DISABLED;
    }

    return EnablementType::DEFAULT;
}

bool is_test_enabled(EnablementType global, EnablementType suite, EnablementType test)
{
    if (test == EnablementType::ENABLED) {
        return true;
    }
    else if (test == EnablementType::DISABLED) {
        return false;
    }
    else if (suite == EnablementType::ENABLED) {
        return true;
    }
    else if (suite == EnablementType::DISABLED) {
        return false;
    }
    else if (global == EnablementType::ENABLED) {
        return true;
    }
    else if (global == EnablementType::DISABLED) {
        return false;
    }
    else {
        return true; // enabled by default
    }
}


}

void run_tests()
{
    filesystem::path config_path = get_tests_config_path();
    U16String config_file = config_path.to_u16();

    YAML::Node tests_config;

    if (filesystem::is_regular_file(config_path))
    {
        reactor::FileInputStream<char> config(open_buffered_file(config_path, FileFlags::RDONLY));
        tests_config = YAML::Load(config);
    }

    auto global_enabled = is_enabled(tests_config);
    const auto& suites  = tests_registry().suites();

    for (auto& suite: suites)
    {
        auto suite_node     = tests_config[suite.first.to_u8().data()];
        auto suite_enabled  = is_enabled(suite_node);

        std::vector<U16String> failed;
        std::vector<U16String> crashed;
		
        int32_t tests_run{};
        int32_t passed{};

        for (auto& test: suite.second->tests())
        {
            auto test_node     = suite_node[test.first.to_u8().data()];
            auto test_enabled  = is_enabled(test_node);

            if (is_test_enabled(global_enabled, suite_enabled, test_enabled))
            {
                tests_run++;

                U16String test_path = suite.first + u"/" + test.first;

                reactor::Process process = reactor::ProcessBuilder::create(reactor::get_program_path())
                        .with_args(U16String("tests2 --test ") + test_path + (config_file.is_empty() ? u"" : U16String(u" --config ") + config_file))
                        .run();

                reactor::InputStreamReader out_reader = process.out_stream();
                reactor::InputStreamReader err_reader = process.err_stream();

                process.join();

                out_reader.join();
                err_reader.join();

                auto status = process.status();

                if (!(status == reactor::Process::Status::EXITED && process.exit_code() == 0))
                {
                    if (status == reactor::Process::Status::CRASHED)
                    {
                        crashed.push_back(test.first);
                    }
                    else {
                        failed.push_back(test.first);
                    }
                }
                else {
                    passed++;
                }
            }
        }

        if (tests_run > 0)
        {
            if (failed.size() == 0 && crashed.size() == 0)
            {
                reactor::engine().coutln(u"{}: PASSED ({})", suite.first, passed);
            }
            else if (failed.size() > 0 && crashed.size() > 0)
            {
                reactor::engine().coutln(u"{}: PASSED ({}); FAILED {}; CRASHED {}", suite.first, passed, to_string(failed), to_string(crashed));
            }
            else if (failed.size() > 0) {
                reactor::engine().coutln(u"{}: PASSED ({}); FAILED {}", suite.first, passed, to_string(failed));
            }
            else {
                reactor::engine().coutln(u"{}: PASSED ({}); CRASHED {}", suite.first, passed, to_string(crashed));
            }
        }
    }
}

}}}
