
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

#include <memoria/tests/runner.hpp>
#include <memoria/reactor/process.hpp>
#include <memoria/reactor/pipe_streams_reader.hpp>
#include <memoria/reactor/application.hpp>
#include <memoria/reactor/file_streams.hpp>
#include <memoria/filesystem/operations.hpp>

#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/linked/document/linked_document.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>



#include <yaml-cpp/yaml.h>
#include <sstream>
#include <thread>

namespace memoria {
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
    catch (MemoriaError& th) {
        th.describe(out);
    }
    catch (std::exception& th) {
        out << "STD Exception: " << th.what() << std::endl;
    }
    catch (boost::exception& th)
    {
        out << boost::diagnostic_information(th);
        out << std::flush;
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
        config_path = app.options()["config"].as<std::string>();
    }
    else {
        auto program_path = reactor::get_program_path();
        config_path = program_path.parent_path();
        config_path.append(get_image_name().string() + ".yaml");
    }

    return config_path;
}


filesystem::path get_tests_output_path()
{
    auto& app = reactor::app();

    filesystem::path output_path;

    if (app.options().count("output") > 0)
    {
        output_path = app.options()["output"].as<std::string>();
    }
    else {
        auto image_name = get_image_name();
        output_path = image_name.string() + ".out";
    }

    return output_path;
}

U8String get_test_coverage_str()
{
    if (app().options().count("coverage") > 0) {
        return U8String(app().options()["coverage"].as<std::string>());
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



TestStatus run_single_test(const U8String& test_path)
{
    auto test = tests_registry().find_test(test_path);
    if (test)
    {
        filesystem::path output_dir_base = get_tests_output_path();
        filesystem::path config_path     = get_tests_config_path();

        U8String suite_name;
        U8String test_name;

        std::tie(suite_name, test_name) = TestsRegistry::split_path(test_path);

        YAML::Node test_config;

        int64_t seed = getTimeInMillis();

        if (filesystem::is_regular_file(config_path))
        {
            FileInputStream<char> fi_stream(open_buffered_file(config_path, FileFlags::RDONLY));
            YAML::Node config = YAML::Load(fi_stream);

            if (config["seed"])
            {
                seed = config["seed"].as<int64_t>();
            }

            YAML::Node suite_node = config[suite_name.to_u8().to_std_string()];
            if (suite_node)
            {
                test_config = suite_node[test_name.to_u8().to_std_string()];
            }
        }

        reactor::engine().coutln("seed = {}", seed);

        Seed(seed);
        SeedBI(seed);

        filesystem::path test_output_dir = output_dir_base;
        test_output_dir.append(suite_name.to_u8().to_std_string());
        test_output_dir.append(test_name.to_u8().to_std_string());

		filesystem::create_directories(test_output_dir);

        Optional<TestCoverage> coverage = get_test_coverage();

        DefaultTestContext ctx(
            test_config,
            config_path.parent_path(),
            filesystem::absolute(test_output_dir),
            coverage.get(),
            false,
            seed
        );

        int64_t start_time = getTimeInMillis();
        test.get().run(&ctx);
        int64_t end_time = getTimeInMillis();

        if (ctx.status() == TestStatus::PASSED)
        {
            reactor::engine().coutln("PASSED in {}s", FormatTime(end_time - start_time));
        }
        else {
            reactor::engine().coutln("FAILED in {}s", FormatTime(end_time - start_time));
            if (ctx.ex())
            {
                dump_exception(ctx.out(), ctx.ex());
            }
        }

        return ctx.status();
    }
    else {
        MMA_THROW(TestConfigurationException()) << format_ex("No test with path '{}' found", test_path);
    }
}


TestStatus replay_single_test(const U8String& test_path)
{
    auto test = tests_registry().find_test(test_path);
    if (test)
    {
        filesystem::path output_dir_base = get_tests_output_path();

        U8String suite_name;
        U8String test_name;

        std::tie(suite_name, test_name) = TestsRegistry::split_path(test_path);

        filesystem::path test_output_dir = output_dir_base;
        test_output_dir.append(suite_name.to_u8().to_std_string());
        test_output_dir.append(test_name.to_u8().to_std_string());

        filesystem::path config_path = test_output_dir;
        config_path.append("config.yaml");

        YAML::Node config;

        int64_t seed = getTimeInMillis();

        if (filesystem::is_regular_file(config_path))
        {
            reactor::FileInputStream<char> fi_config(open_buffered_file(config_path, FileFlags::RDONLY));
            config = YAML::Load(fi_config);

            if (config["seed"])
            {
                seed = config["seed"].as<int64_t>();
            }
        }

        reactor::engine().coutln("seed = {}", seed);
        Seed(static_cast<int32_t>(seed));
        SeedBI(seed);

        Optional<TestCoverage> coverage = get_test_coverage();

        DefaultTestContext ctx(
            config,
            config_path.parent_path(),
            filesystem::absolute(test_output_dir),
            coverage.get(),
            true,
            seed
        );

        int64_t start_time = getTimeInMillis();
        test.get().run(&ctx);
        int64_t end_time = getTimeInMillis();

        if (ctx.status() == TestStatus::PASSED)
        {
            reactor::engine().coutln("PASSED in {}s", FormatTime(end_time - start_time));
        }
        else {
            reactor::engine().coutln("FAILED in {}s", FormatTime(end_time - start_time));
            if (ctx.ex())
            {
                dump_exception(ctx.out(), ctx.ex());
            }
        }

        return ctx.status();
    }
    else {
        MMA_THROW(TestConfigurationException()) << format_ex("No test with path '{}' found", test_path);
    }
}



template <typename T>
U8String to_string(const std::vector<T>& array)
{
    U8String data = "[";

    bool first = true;

    for (const auto& ee : array)
    {
        if (!first) {
            data += ", ";
        }
        else {
            first = false;
        }

        std::stringstream ss;
        ss << ee;

        data += U8String(ss.str());
    }

    data += "]";
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
    U8String config_file           = U8String(config_path.string());

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

        std::vector<U8String> failed;
        std::vector<U8String> crashed;
		
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

                U8String test_path = suite.first + "/" + test.first;

                std::vector<U8String> args;

                args.emplace_back("tests2");
                args.emplace_back("--test");
                args.emplace_back(test_path);

                if (!config_file.is_empty()) {
                    args.emplace_back("--config");
                    args.emplace_back(config_file);
                }

                if (test_node["threads"])
                {
                    args.emplace_back("-t");
                    args.emplace_back(test_node["threads"].as<std::string>());
                }
                else {
                    auto state = test.second->create_state();
                    args.emplace_back("-t");
                    args.emplace_back(std::to_string(state->threads()));
                }

                args.emplace_back("--output");
                args.emplace_back(U8String(output_dir_base.string()));

                args.emplace_back("--coverage");
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
                reactor::engine().coutln("{}: PASSED ({})", suite.first, passed);
            }
            else if (failed.size() > 0 && crashed.size() > 0)
            {
                reactor::engine().coutln("{}: PASSED ({}); FAILED {}; CRASHED {}", suite.first, passed, to_string(failed), to_string(crashed));
            }
            else if (failed.size() > 0) {
                reactor::engine().coutln("{}: PASSED ({}); FAILED {}", suite.first, passed, to_string(failed));
            }
            else {
                reactor::engine().coutln("{}: PASSED ({}); CRASHED {}", suite.first, passed, to_string(crashed));
            }
        }
    }
}

void Test::run(TestContext *context) noexcept
{
    std::unique_ptr<TestState> state;

    try {
        state = create_state();
        state->set_seed(context->seed());
        state->set_replay(context->is_replay());

        state->working_directory_ = context->data_directory();

        state->pre_configure(context->coverage());
        state->add_field_handlers();
        state->add_indirect_field_handlers();
        state->post_configure(context->coverage());

        CommonConfigurationContext configuration_context(context->configurator()->config_base_path());

        state->internalize(context->configurator()->configuration(), &configuration_context);

        state->set_up();
    }
    catch (...) {
        context->failed(TestStatus::SETUP_FAILED, std::current_exception(), state.get());
        return;
    }

    try {
        if (context->is_replay()) {
            replay_test(state.get());
        }
        else {
            test(state.get());
        }

        state->tear_down();
        context->passed();
    }
    catch (...) {
        context->failed(TestStatus::TEST_FAILED, std::current_exception(), state.get());
    }
}

void Test::replay_test(TestState* state) {
    MMA_THROW(TestException()) << WhatCInfo("No Replay method exists for the test requested");
}

void DefaultTestContext::failed(TestStatus detail, std::exception_ptr ex, TestState* state) noexcept
{
    status_ = detail;
    ex_ = ex;

    if (detail == TestStatus::TEST_FAILED && !state->is_replay())
    {
        state->on_test_failure();

        try {
            filesystem::path config_path = this->data_directory();
            config_path.append("config.yaml");

            CommonConfigurationContext configuration_context(data_directory());
            YAML::Node config;

            state->externalize(config, &configuration_context);

            FileOutputStream<char> stream(open_buffered_file(config_path, FileFlags::RDWR | FileFlags::CREATE | FileFlags::TRUNCATE));
            YAML::Emitter emitter(stream);

            emitter.SetIndent(4);
            emitter << config;

            stream.flush();
        }
        catch (...) {
            println("Can't externalize the test's state");
        }
    }
}



void MultiProcessRunner::start()
{
    socket_ = ServerSocket(address_, port_);
    socket_.listen();

    address_ = socket_.address();
    port_ = socket_.port();

    worker_processes_.resize(workers_num_);

    for (size_t c = 0; c < workers_num_; c++) {
        worker_processes_[c] = create_worker(c);        
    }

    for (size_t c = 0; c < workers_num_; c++) {
        fibers_.push_back(std::make_pair(format_u8("Process Watcher {}", c), worker_processes_[c]->start()));
    }

    handle_connections();
}

std::shared_ptr<WorkerProcess> MultiProcessRunner::create_worker(size_t num)
{
    filesystem::path output_dir_base = get_tests_output_path();
    output_dir_base.append(std::to_string(num));
    filesystem::create_directories(output_dir_base);

    auto proc = std::make_shared<WorkerProcess>(address_, port_, output_dir_base, num);

    proc->set_status_listener([weak_self = weak_from_this(), address = address_, port = port_, num, output_dir_base, this](auto status, auto exit_code)
    {
        auto self = weak_self.lock();
        if (!self) {
            return;
        }

        bool restart{false};

        if (status != reactor::Process::Status::EXITED || (status == reactor::Process::Status::EXITED && exit_code != 0))
        {
            auto ii = self->heads_.find(num);
            if (ii != self->heads_.end()) {
                //engine().
                println("CRASHED: {} :: {}", ii->second, num);
                self->heads_.erase(ii);
                self->crashes_++;
            }

            restart = true;
        }
        else {
            self->heads_.erase(num);
            restart = exit_code != 0;
        }

        if (restart && respawn_workers_)
        {
            self->worker_processes_[num] = std::make_shared<WorkerProcess>(address, port, output_dir_base, num);
            self->fibers_.push_back(std::make_pair(format_u8("Process Watcher {}", num), self->worker_processes_[num]->start()));
        }
    });

    return proc;
}

void write_message(BinaryOutputStream output, const U8StringView& msg)
{
    uint64_t size = msg.size();
    output.write(ptr_cast<const uint8_t>(&size), sizeof(size));
    output.write(ptr_cast<const uint8_t>(msg.data()), size);
    output.flush();
}

LDDocument read_message(BinaryInputStream input)
{
    uint64_t size{0};
    if (input.read(ptr_cast<uint8_t>(&size), sizeof(size)) < sizeof(size))
    {
        MEMORIA_MAKE_GENERIC_ERROR("Connection has been closed").do_throw();
    }

    U8String str(size, ' ');
    if (input.read(ptr_cast<uint8_t>(str.data()), size) < size) {
        MEMORIA_MAKE_GENERIC_ERROR("Connection has been closed").do_throw();
    }

    return LDDocument::parse(str);
}



void MultiProcessRunner::handle_connections()
{
    const auto& suites  = tests_registry().suites();

    std::list<U8String> tests;


    for (const auto& suite: suites)
    {
        for (const auto& test: suite.second->tests()) {
            U8String full_test_name = suite.first + "/" + test.first;
            tests.push_back(full_test_name);
        }
    }

    size_t total_tests = tests.size();
    size_t processed{};
    size_t sent{};

    std::set<U8String> sent_tasks;
    std::set<U8String> processed_tasks;


    bool do_finish = false;

    while (processed + crashes_ < total_tests)
    {
        fibers::fiber ff([&](SocketConnectionData&& conn_data) {

            std::shared_ptr<WorkerProcess> worker_process;
            size_t worker_num = -1ull;

            try {
                ServerSocketConnection conn(std::move(conn_data));
                BinaryInputStream input  = conn.input();
                BinaryOutputStream output = conn.output();

                while (tests.size() || heads_.count(worker_num) > 0)
                {
                    LDDocument msg = read_message(input);
                    U8String code = get_value(msg.value(), "code").as_varchar().view();

                    if (code == "GREETING")
                    {
                        worker_num = get_value(msg.value(), "worker_id").as_bigint();
                        worker_process = worker_processes_.at(worker_num);
                    }
                    else if (code == "GET_TASK")
                    {
                        if (tests.size()) {
                            U8String new_test = tests.front();
                            tests.pop_front();

                            heads_[worker_num] = new_test;

                            sent_tasks.insert(new_test);
                            write_message(output, format_u8("{{'code': 'RUN_TASK', 'test_path': '{}'}}", new_test));
                            sent++;
                        }
                        else {
                            break;
                        }
                    }
                    else if (code == "TASK_RESULT")
                    {
                        processed++;

                        U8String test_path = get_value(msg.value(), "test_path").as_varchar().view();
                        int32_t status = get_value(msg.value(), "status").as_bigint();

                        heads_.erase(worker_num);

                        processed_tasks.insert(test_path);

                        if (status > 0) {
                            //engine().
                            println("*FAILED: {} :: {}({}) of {}", test_path, processed + crashes_, crashes_, total_tests);
                        }
                        else {
                            //engine().
                            println("PASSED: {} :: {}({}) of {}", test_path, processed + crashes_, crashes_, total_tests);
                        }
                    }
                }

                if (worker_num != -1ull) {
                    write_message(output, "{'code': 'TERMINATE'}");
                }

                if (processed + crashes_ >= total_tests) {
                    if (!do_finish) {
                        do_finish = true;
                        for (size_t c = 0; c < 3; c++) {
                            //println("Ping {} of {}", c + 1, workers_num_);
                            ping_socket();
                        }
                    }
                }
            }
            catch (const std::exception& ex)
            {
                if (worker_process) {
                    worker_process->terminate();
                    worker_process->join();
                }
            }
            catch (const std::runtime_error& ex)
            {
                if (worker_process) {
                    worker_process->terminate();
                    worker_process->join();
                }
            }
            catch (const MemoriaThrowable& ex)
            {
                if (worker_process) {
                    worker_process->terminate();
                    worker_process->join();
                }
            }
            catch (...) {
                engine().println("Unknown exception, worker = {}", worker_num);
            }
        }, socket_.accept());

        fibers_.push_back(std::make_pair(format_u8("WorkerFacacde"), std::move(ff)));
    }

    respawn_workers_ = false;

    for (auto proc: worker_processes_) {
        proc->join();
    }

    for (auto& pair: fibers_)
    {
        if (pair.second.joinable()) {

            pair.second.join();
        }
    }

    socket_.close();
}


void MultiProcessRunner::ping_socket()
{
    reactor::ClientSocket c_socket(address_, port_);
    BinaryOutputStream output = c_socket.output();

    write_message(output, "{'code': 'NONE'}");

    output.close();
    c_socket.close();
}


reactor::Process::Status WorkerProcess::status() const {
    if (process_) {
        return process_.status();
    }
    else {
        return reactor::Process::Status::TERMINATED;
    }
}

WorkerProcess::~WorkerProcess() noexcept {}

fibers::fiber WorkerProcess::start()
{
    filesystem::path output_dir_base(output_folder_);

    std::vector<U8String> args;

    args.emplace_back("tests2");
    args.emplace_back("--server");
    args.emplace_back(socket_addr_.to_string());
    args.emplace_back("--port");
    args.emplace_back(std::to_string(port_));

    args.emplace_back("--worker-num");
    args.emplace_back(std::to_string(worker_num_));

    args.emplace_back("--output");
    args.emplace_back(U8String(output_dir_base.string()));

    args.emplace_back("--coverage");
    args.emplace_back(get_test_coverage_str());

    process_ = reactor::ProcessBuilder::create(reactor::get_program_path())
            .with_args(args)
            .run();

    filesystem::path std_output = output_dir_base;
    std_output.append("stdout.txt");
    out_file_ = open_buffered_file(std_output, FileFlags::CREATE | FileFlags::RDWR | FileFlags::TRUNCATE);

    filesystem::path std_error = output_dir_base;
    std_error.append("stderr.txt");
    err_file_ = open_buffered_file(std_error, FileFlags::CREATE | FileFlags::RDWR | FileFlags::TRUNCATE);

    out_reader_ = std::make_unique<reactor::InputStreamReaderWriter>(process_.out_stream(), out_file_.ostream());
    err_reader_ = std::make_unique<reactor::InputStreamReaderWriter>(process_.err_stream(), err_file_.ostream());

    return fibers::fiber([&]{
        auto holder = this->shared_from_this();

        process_.join();

        out_reader_->join();
        err_reader_->join();

        out_file_.close();
        err_file_.close();

        auto status = process_.status();

        int32_t code{-1};

        if (status == reactor::Process::Status::EXITED) {
            code = process_.exit_code();
        }

        if (status_listener_) {
            //engine().println("Process {} exited with status {}, code {}", worker_num_, (int)status, code);
            status_listener_(status, code);
        }
    });
}





void Worker::run()
{
    socket_ = reactor::ClientSocket(server_address_, port_);

    BinaryInputStream input = socket_.input();
    BinaryOutputStream output = socket_.output();

    write_message(output, format_u8("{{'code': 'GREETING', 'worker_id':{}}}", worker_num_));

    while (true)
    {
        write_message(output, "{'code': 'GET_TASK'}");

        LDDocument msg = read_message(input);
        U8String code = get_value(msg.value(), "code").as_varchar().view();

        if (code == "RUN_TASK")
        {            
            U8String test_path = get_value(msg.value(), "test_path").as_varchar().view();

            println("++++++++++ New message from server: {}", test_path);

            TestStatus status = run_single_test(test_path);

            write_message(output, format_u8(
R"({{
    'code': 'TASK_RESULT',
    'test_path': '{}',
    'status': {}
}})", test_path, (int)status));
        }
        else {
            input.close();
            output.close();
            socket_.close();
            return;
        }
    }
}

void run_tests2(size_t threads)
{
    auto runner = std::make_shared<MultiProcessRunner>(threads);
    runner->start();
}

}}
