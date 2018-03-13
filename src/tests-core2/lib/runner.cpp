
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

#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

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


filesystem::path get_tests_output_path()
{
    auto& app = reactor::app();

    filesystem::path output_path;

    if (app.options().count("output") > 0)
    {
        output_path = U16String(app.options()["output"].as<std::string>());
    }
    else {
        auto image_name = get_image_name();
        output_path = image_name.to_u16() + u".out";
    }

    return output_path;
}

U16String get_test_coverage_str()
{
    if (app().options().count("coverage") > 0) {
        return U16String(app().options()["coverage"].as<std::string>());
    }
    else {
        return "small";
    }
}

Optional<TestCoverage> get_test_coverage()
{
    if (app().options().count("coverage") > 0) {
        return coverage_from_string(get_test_coverage_str().to_u8());
    }
    else {
        return TestCoverage::SMALL;
    }
}


}

TestStatus run_single_test(const U16String& test_path)
{
    auto test = tests_registry().find_test(test_path);
    if (test)
    {
        filesystem::path output_dir_base = get_tests_output_path();
        filesystem::path config_path     = get_tests_config_path();

        U16String suite_name;
        U16String test_name;

        std::tie(suite_name, test_name) = TestsRegistry::split_path(test_path);

        YAML::Node test_config;

        if (filesystem::is_regular_file(config_path))
        {
            YAML::Node config = YAML::LoadFile(config_path.to_u8().to_std_string());

            if (config["seed"])
            {
                int64_t seed = config["seed"].as<int64_t>();
                Seed(seed);
                SeedBI(seed);
            }
            else {
                Seed(getTimeInMillis());
                SeedBI(getTimeInMillis());
            }

            YAML::Node suite_node = config[suite_name.to_u8().to_std_string()];
            if (suite_node)
            {
                test_config = suite_node[test_name.to_u8().to_std_string()];
            }
        }
        else
        {
            int64_t seed = getTimeInMillis();
            Seed(seed);
            SeedBI(seed);
        }

        filesystem::path test_output_dir = output_dir_base;
        test_output_dir.append(suite_name.to_u8().to_std_string());
        test_output_dir.append(test_name.to_u8().to_std_string());

        Optional<TestCoverage> coverage = get_test_coverage();

        DefaultTestContext ctx(test_config, config_path.parent_path(), filesystem::absolute(test_output_dir), coverage.get());

        int64_t start_time = getTimeInMillis();
        test.get().run(&ctx);
        int64_t end_time = getTimeInMillis();


        if (ctx.status() == TestStatus::PASSED)
        {
            reactor::engine().coutln(u"PASSED in {}s", FormatTime(end_time - start_time));
        }
        else {
            reactor::engine().coutln(u"FAILED in {}s", FormatTime(end_time - start_time));
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
    filesystem::path output_dir_base = get_tests_output_path();

    filesystem::path config_path    = get_tests_config_path();
    U16String config_file           = config_path.to_u16();

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

                filesystem::path test_output_dir = output_dir_base;
                test_output_dir.append(suite.first.to_u8().to_std_string());
                test_output_dir.append(test.first.to_u8().to_std_string());

                filesystem::create_directories(test_output_dir);

                U16String test_path = suite.first + u"/" + test.first;

                std::vector<U16String> args;

                args.emplace_back(u"tests2");
                args.emplace_back(u"--test");
                args.emplace_back(test_path);

                if (!config_file.is_empty()) {
                    args.emplace_back(u"--config");
                    args.emplace_back(config_file);
                }

                if (test_node["threads"])
                {
                    args.emplace_back(u"-t");
                    args.emplace_back(test_node["threads"].as<std::string>());
                }
                else {
                    auto state = test.second->create_state();
                    args.emplace_back(u"-t");
                    args.emplace_back(std::to_string(state->threads()));
                }

                args.emplace_back(u"--output");
                args.emplace_back(output_dir_base.to_u16());

                args.emplace_back(u"--coverage");
                args.emplace_back(get_test_coverage_str());

                reactor::Process process = reactor::ProcessBuilder::create(reactor::get_program_path())
                        .with_args(args)
                        .run();

                filesystem::path std_output = test_output_dir;
                std_output.append("stdout.txt");
                File out_file = open_buffered_file(std_output, FileFlags::CREATE | FileFlags::RDWR | FileFlags::TRUNCATE);

                filesystem::path std_error = test_output_dir;
                std_error.append("stderr.txt");
                File err_file = open_buffered_file(std_error, FileFlags::CREATE | FileFlags::RDWR | FileFlags::TRUNCATE);

                reactor::InputStreamReaderWriter out_reader(process.out_stream(), out_file.ostream());
                reactor::InputStreamReaderWriter err_reader(process.err_stream(), err_file.ostream());

                process.join();

                out_reader.join();
                err_reader.join();

                out_file.close();
                err_file.close();

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

void Test::run(TestContext *context) noexcept
{
    std::unique_ptr<TestState> state;

    try {
        state = create_state();

        state->working_directory_ = context->data_directory();

        state->pre_configure(context->coverage());
        state->add_field_handlers();
        state->add_indirect_field_handlers();
        state->post_configure(context->coverage());

        CommonConfigurationContext configuration_context(context->configurator()->config_base_path());

        state->internalize(context->configurator()->configuration(), &configuration_context);
    }
    catch (...) {
        context->failed(TestStatus::SETUP_FAILED, std::current_exception());
        return;
    }

    try {
        test(state.get());
        context->passed();
    }
    catch (...) {
        context->failed(TestStatus::TEST_FAILED, std::current_exception());
    }
}


}}}
