
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

#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/hermes/hermes.hpp>

#include <boost/fiber/all.hpp>
#include <memoria/asio/round_robin.hpp>
#include <memoria/asio/yield.hpp>
#include <boost/shared_ptr.hpp>

#include <yaml-cpp/yaml.h>
#include <sstream>
#include <thread>
#include <functional>

namespace memoria {
namespace tests {

namespace bp = boost::process;

using fs::path;

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

fs::path get_tests_config_path()
{
    auto& config = get_config();

    fs::path config_path;

    if (config.count("config") > 0)
    {
        config_path = config["config"].as<std::string>();
    }
    else {
        fs::path program_path = get_program_path();
        config_path = program_path.parent_path();
        config_path.append(get_image_name().string() + ".yaml");
    }

    return config_path;
}


fs::path get_tests_output_path()
{
    auto& config = get_config();

    fs::path output_path;

    if (config.count("output") > 0)
    {
        output_path = config["output"].as<std::string>();
    }
    else {
        auto image_name = get_image_name();
        output_path = image_name.string() + ".out";
    }

    return output_path;
}

U8String get_test_coverage_str()
{
    auto& config = get_config();

    if (config.count("coverage") > 0) {
        return U8String(config["coverage"].as<std::string>());
    }
    else {
        return "small";
    }
}

Optional<TestCoverage> get_test_coverage()
{
    auto& config = get_config();

    if (config.count("coverage") > 0) {
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
        fs::path output_dir_base = get_tests_output_path();
        fs::path config_path     = get_tests_config_path();

        U8String suite_name;
        U8String test_name;

        std::tie(suite_name, test_name) = TestsRegistry::split_path(test_path);

        YAML::Node test_config;

        int64_t seed = getTimeInMillis();

        if (fs::is_regular_file(config_path))
        {
            YAML::Node config = YAML::Load(config_path.string());

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

        println("seed = {}", seed);

        Seed(seed);
        SeedBI(seed);

        fs::path test_output_dir = output_dir_base;
        test_output_dir.append(suite_name.to_u8().to_std_string());
        test_output_dir.append(test_name.to_u8().to_std_string());

        fs::create_directories(test_output_dir);

        Optional<TestCoverage> coverage = get_test_coverage();

        DefaultTestContext ctx(
            test_config,
            config_path.parent_path(),
            fs::absolute(test_output_dir),
            coverage.value(),
            false,
            seed
        );

        int64_t start_time = getTimeInMillis();
        test->run(&ctx);
        int64_t end_time = getTimeInMillis();

        if (ctx.status() == TestStatus::PASSED)
        {
            println("PASSED in {}s", FormatTime(end_time - start_time));
        }
        else {
            println("FAILED in {}s", FormatTime(end_time - start_time));
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
        fs::path output_dir_base = get_tests_output_path();

        U8String suite_name;
        U8String test_name;

        std::tie(suite_name, test_name) = TestsRegistry::split_path(test_path);

        fs::path test_output_dir = output_dir_base;
        test_output_dir.append(suite_name.to_u8().to_std_string());
        test_output_dir.append(test_name.to_u8().to_std_string());

        fs::path config_path = test_output_dir;
        config_path.append("config.yaml");

        YAML::Node config;

        int64_t seed = getTimeInMillis();

        if (fs::is_regular_file(config_path))
        {
            config = YAML::Load(config_path.string());

            if (config["seed"])
            {
                seed = config["seed"].as<int64_t>();
            }
        }

        println("seed = {}", seed);
        Seed(static_cast<int32_t>(seed));
        SeedBI(seed);

        Optional<TestCoverage> coverage = get_test_coverage();

        DefaultTestContext ctx(
            config,
            config_path.parent_path(),
            fs::absolute(test_output_dir),
            coverage.value(),
            true,
            seed
        );

        int64_t start_time = getTimeInMillis();
        test->run(&ctx);
        int64_t end_time = getTimeInMillis();

        if (ctx.status() == TestStatus::PASSED)
        {
            println("PASSED in {}s", FormatTime(end_time - start_time));
        }
        else {
            println("FAILED in {}s", FormatTime(end_time - start_time));
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
            fs::path config_path = this->data_directory();
            config_path.append("config.yaml");

            CommonConfigurationContext configuration_context(data_directory());
            YAML::Node config;

            state->externalize(config, &configuration_context);

            std::fstream s;
            s.open(config_path.string(), s.binary | s.trunc | s.in | s.out);

            YAML::Emitter emitter(s);

            emitter.SetIndent(4);
            emitter << config;

            s.flush();
        }
        catch (...) {
            println("Can't externalize the test's state");
        }
    }
}



void MultiProcessRunner::start()
{
//    ss::listen_options opts;
//    opts.reuse_address = true;

    const auto& suites  = tests_registry().suites();

    for (const auto& suite: suites)
    {
        for (const auto& test: suite.second->tests()) {
            U8String full_test_name = suite.first + "/" + test.first;
            tests_.push_back(full_test_name);
        }
    }

//    socket_ = ss::listen(
//        ss::socket_address(ss::ipv4_addr(address_.to_std_string(), port_)),
//        opts
//    );

    if (tests_.size() < workers_num_) {
        workers_num_ = tests_.size();
    }

    println("Starting {} workers", workers_num_);
    worker_processes_.resize(workers_num_);

    for (size_t c = 0; c < workers_num_; c++) {
        worker_processes_[c] = create_worker(c);        
    }

    for (size_t c = 0; c < workers_num_; c++) {
        threads_.push_back(std::make_pair(format_u8("Process Watcher {}", c), worker_processes_[c]->start()));
    }

    handle_connections();
}

std::shared_ptr<WorkerProcess> MultiProcessRunner::create_worker(size_t num)
{
    fs::path output_dir_base = get_tests_output_path();
    output_dir_base.append(std::to_string(num));
    fs::create_directories(output_dir_base);

    auto proc = std::make_shared<WorkerProcess>(address_, 0, output_dir_base, num);

    proc->set_status_listener([weak_self = weak_from_this(), address = address_, port = port_, num, output_dir_base, this](auto status, auto exit_code)
    {
        auto self = weak_self.lock();
        if (!self) {
            return;
        }

        bool restart{false};
        if (status != Status::EXITED || (status == Status::EXITED && exit_code != 0))
        {
            auto ii = self->heads_.find(num);
            if (ii != self->heads_.end()) {

                if (status == Status::CRASHED) {
                    println("CRASHED: {} :: {}", ii->second, num);
                    self->crashes_++;
                }
                else if (status == Status::UNKNOWN) {
                    println("UNKNOWN: {} :: {}", ii->second, num);
                }
                else if (status == Status::TERMINATED) {
                    println("TERMINATED: {} :: {}", ii->second, num);
                }

                self->heads_.erase(ii);
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
            self->threads_.push_back(std::make_pair(format_u8("Process Watcher {}", num), self->worker_processes_[num]->start()));
        }
    });

    return proc;
}

bool write_data(net::ip::tcp::socket& socket, const void* data, size_t size)
{
    boost::system::error_code ec;
    net::async_write(
            socket,
            net::buffer(data, size),
            memoria::asio::yield[ec]);

    if ( ec == net::error::eof) {
        return false;
    }
    else if (ec) {
        throw boost::system::system_error(ec);
    }
    return true;
}

size_t read_data(net::ip::tcp::socket& socket, uint8_t* data, size_t size)
{
    boost::system::error_code ec;
    size_t reply_length = socket.async_read_some(
            boost::asio::buffer(data, size),
            memoria::asio::yield[ec]);

    if (ec == boost::asio::error::eof) {
        return 0;
    }
    else if (ec) {
        throw boost::system::system_error(ec);
    }

    return reply_length;
}


bool read_data_fully(net::ip::tcp::socket& socket, void* data, size_t size)
{
    uint8_t* udata = reinterpret_cast<uint8_t*>(data);
    size_t cnt = 0;
    while (cnt < size)
    {
        size_t rr = read_data(socket, udata + cnt, size - cnt);
        if (rr == 0) {
            return false;
        }
        cnt += rr;
    }
    return cnt;
}

bool write_message(net::ip::tcp::socket& socket, const U8StringView& msg)
{
    uint64_t size = msg.size();
    if (write_data(socket, &size, sizeof(size))) {
        return write_data(socket, msg.data(), msg.size());
    }
    return false;
}

hermes::HermesCtr read_message(net::ip::tcp::socket& socket)
{
    uint64_t size{};
    if (read_data_fully(socket, &size, sizeof(size)))
    {
        auto buf = allocate_system_zeroed<char>(size + 1);
        if (read_data_fully(socket, buf.get(), size)) {
            return hermes::HermesCtr::parse_document(U8String(buf.get(), size));
        }
    }

    return {};
}



template <typename StreamT>
struct StreamCloser {
    StreamT* stream_{};
    ~StreamCloser() {
        if (stream_) {
            stream_->close().get();
        }
    }
};


void MultiProcessRunner::handle_connections()
{
    size_t total_tests = tests_.size();
    size_t processed{};
    size_t sent{};

    std::set<U8String> sent_tasks;
    std::set<U8String> processed_tasks;

    using SocketPtr = std::unique_ptr<net::ip::tcp::socket>;

    while (processed + crashes_ < total_tests)
    {
        auto fn = [&](SocketPtr conn) {
            std::shared_ptr<WorkerProcess> worker_process;
            size_t worker_num = -1ull;

            try {
                //auto input  = conn_data.connection.input();
                //auto output = conn_data.connection.output();

                //StreamCloser<ss::input_stream<char>> is_closer{&input};
                //StreamCloser<ss::output_stream<char>> os_closer{&output};

                while (tests_.size() || heads_.count(worker_num) > 0)
                {
                    auto msg = read_message(*conn);
                    U8String code = msg.root().value().search("code").as_varchar();

                    if (code == "GREETING")
                    {
                        worker_num = msg.root().value().search("worker_id").to_i64();
                        worker_process = worker_processes_.at(worker_num);
                    }
                    else if (code == "GET_TASK")
                    {
                        if (tests_.size()) {
                            U8String new_test = tests_.front();
                            tests_.pop_front();

                            heads_[worker_num] = new_test;

                            sent_tasks.insert(new_test);
                            write_message(*conn, format_u8("{{'code': 'RUN_TASK', 'test_path': '{}'}}", new_test));
                            sent++;
                        }
                        else {
                            break;
                        }
                    }
                    else if (code == "TASK_RESULT")
                    {
                        processed++;

                        U8String test_path = msg.root().value().search("test_path").as_varchar();
                        int32_t status = msg.root().value().search("status").to_i32();

                        heads_.erase(worker_num);

                        processed_tasks.insert(test_path);

                        if (status > 0) {
                            println("*FAILED: {} :: {}({}) of {}", test_path, processed + crashes_, crashes_, total_tests);
                        }
                        else {
                            println("PASSED: {} :: {}({}) of {}", test_path, processed + crashes_, crashes_, total_tests);
                        }
                    }
                }

                if (worker_num != -1ull) {
                    write_message(*conn, "{'code': 'TERMINATE'}");
                }

                if (processed + crashes_ >= total_tests) {
                    socket_.close();
                }
            }
            catch (const std::system_error& ex)
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
            catch (const std::runtime_error& ex)
            {
                if (worker_process) {
                    worker_process->terminate();
                    worker_process->join();
                }
            }
            catch (const std::exception& ex)
            {
                if (worker_process) {
                    worker_process->terminate();
                    worker_process->join();
                }
            }            
            catch (...) {
                println("Unknown exception, worker = {}", worker_num);
            }
        };

        SocketPtr conn = std::make_unique<net::ip::tcp::socket>(ios_);

        //ss::thread_attributes ta;
        //ta.stack_size = 1024*1024;
        auto thread = boost::fibers::async(std::move(fn), std::move(conn));

        threads_.push_back(std::make_pair(format_u8("WorkerFacacde"), std::move(thread)));
    }

    respawn_workers_ = false;

    for (auto proc: worker_processes_) {
        try {
            proc->join();
        }
        catch (const std::system_error& err) {
            if (err.code().value() != 10) {
                throw err;
            }
        }
    }

    for (auto& pair: threads_)
    {
        try {
            pair.second.get();
        }
        catch (const std::system_error& err) {
            if (err.code().value() != 10) {
                throw err;
            }
        }
    }
}


void MultiProcessRunner::ping_socket()
{
//    ss::connected_socket c_socket = ss::engine().connect(ss::socket_address(ss::ipv4_addr(address_, port_))).get();
//    auto output = c_socket.output();

//    write_message(output, "{'code': 'NONE'}");

//    output.close().get();
}

WorkerProcess::~WorkerProcess() noexcept {}

boost::fibers::future<void> WorkerProcess::start()
{
    fs::path output_dir_base(output_folder_);

    std::vector<std::string> args;

    args.emplace_back("--server");
    args.emplace_back(socket_addr_);
    args.emplace_back("--port");
    args.emplace_back(std::to_string(port_));

    args.emplace_back("--worker-num");
    args.emplace_back(std::to_string(worker_num_));

    args.emplace_back("--output");
    args.emplace_back(U8String(output_dir_base.string()));

    args.emplace_back("--coverage");
    args.emplace_back(get_test_coverage_str());

    fs::path std_output = output_dir_base;
    std_output.append("stdout.txt");

    fs::path std_error = output_dir_base;
    std_error.append("stderr.txt");

    process_ = std::make_unique<Process>(get_program_path().string(), args, std_output.string(), std_error.string());

    return boost::fibers::async([&]() {
        auto holder = this->shared_from_this();

        int32_t code{-1};
        Status status = Status::EXITED;

        try {
            status = process_->join();
        }
        catch (const std::system_error& err) {
            if (err.code().value() != 3) {
                status = Status::CRASHED;
            }
            else {
                status = Status::UNKNOWN;
            }
        }
        catch (...) {
            status = Status::UNKNOWN;
        }

        if (status == Status::EXITED) {
            code = process_->exit_code();
        }

        if (status_listener_) {
            status_listener_(status, code);
        }
    });
}

void AbstractWorker::handle_messages() {
    try {
        write_message(format_u8("{{'code': 'GREETING', 'worker_id':{}}}", worker_num_));

        while (true)
        {
            write_message("{'code': 'GET_TASK'}");

            auto msg = read_message();
            U8String code = msg.root().value().search("code").as_varchar();

            if (code == "RUN_TASK")
            {
                U8String test_path = msg.root().value().search("test_path").as_varchar();
                TestStatus status = run_single_test(test_path);

                write_message(format_u8(
R"({{
    'code': 'TASK_RESULT',
    'test_path': '{}',
    'status': {}
}})", test_path, (int)status));
            }
            else {
                return;
            }
        }
    }
    catch(const std::system_error& err) {
        if (err.code().value() != 111) {
            throw err;
        }
    }
}


void ASIOWorker::run()
{
    using net::ip::tcp;

    tcp::resolver resolver(ios_);
    tcp::resolver::query query( tcp::v4(), "127.0.0.1", "9999");
    tcp::resolver::iterator iterator = resolver.resolve( query);
    net::connect(socket_, iterator);

    handle_messages();

    socket_.close();
}


bool ASIOWorker::write_message(U8String msg)
{
    return memoria::tests::write_message(socket_, msg);
}


hermes::HermesCtr ASIOWorker::read_message()
{
    return memoria::tests::read_message(socket_);
}



void run_tests2(net::io_service& ios, size_t threads) {
    auto runner = std::make_shared<MultiProcessRunner>(ios, threads);
    runner->start();
}


boost::program_options::variables_map vmap_;

boost::program_options::variables_map& get_config() {
    return vmap_;
}

namespace {

size_t find_zero(const char* mem, size_t max)
{
    size_t c;
    for (c = 0; c < max; c++)
    {
        if (mem[c] == 0) {
            break;
        }
    }
    return c;
}

}

fs::path get_program_path()
{
    const char* link_path = "/proc/self/cmdline";

    std::fstream ff;
    ff.open(link_path, ff.in);

    U8String str;
    while (!ff.eof()) {
        char ch = 0;
        ff >> ch;

        if (ch) {
            str += U8String(1, ch);
        }
        else {
            break;
        }
    }
    ff.close();
    return str.to_std_string();
}

fs::path get_image_name()
{
    return get_program_path().filename();
}




static bp::child start_process(std::string name, std::string stdout, std::string stderr, const std::vector<std::string>& args)
{
    if (args.size() == 0) {
        return bp::child(name, bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 1) {
        return bp::child(
                name,
                args[0],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 2) {
        return bp::child(
                name,
                args[0],
                args[1],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 3) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 4) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 5) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 6) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 7) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 8) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 9) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7],
                args[8],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 10) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7],
                args[8],
                args[9],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 11) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7],
                args[8],
                args[9],
                args[10],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 12) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7],
                args[8],
                args[9],
                args[10],
                args[11],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 13) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7],
                args[8],
                args[9],
                args[10],
                args[11],
                args[12],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else if (args.size() == 14) {
        return bp::child(
                name,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7],
                args[8],
                args[9],
                args[10],
                args[11],
                args[12],
                args[13],
                bp::std_out > stdout, bp::std_err > stderr);
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Supplied number of arguments {} is too big", args.size()).do_throw();
    }
}



std::ostream& operator<<(std::ostream& out, Status status)
{
    switch (status)
    {
        case Status::RUNNING: out << "RUNNING"; break;
        case Status::EXITED:  out << "EXITED"; break;
        case Status::CRASHED: out << "CRASHED"; break;
        case Status::TERMINATED: out << "TERMINATED"; break;
        case Status::UNKNOWN: out << "UNKNOWN"; break;
        case Status::FAILED: out << "FAILED"; break;
    }

    return out;
}


Process::Process(
        std::string filename,
        const std::vector<std::string>& args,
        std::string stdout_fname,
        std::string stderr_fname
    ):
    process_(start_process(filename, stdout_fname, stderr_fname, args))
{
}

void Process::terminate() {
    //ss::engine().kill(process_.id(), SIGTERM);
}

Status Process::join()
{
    int status = 0;//ss::engine().waitpid(process_.id()).get();

    Status ss = Status::EXITED;

    if (WIFSIGNALED(status))
    {
        int sign = WTERMSIG(status);
        if (sign == SIGTERM) {
            ss = Status::TERMINATED;
        }
        else {
            ss = Status::CRASHED;
        }
    }
    else if (WEXITSTATUS(status)) {
        ss = Status::FAILED;
    }

    return ss;
}

}}
