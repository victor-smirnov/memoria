
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

#pragma once

#include <memoria/tests/tests.hpp>

#include <memoria/core/hermes/hermes.hpp>

#ifdef IOCB_FLAG_RESFD
#undef IOCB_FLAG_RESFD
#endif

//#include <seastar/core/app-template.hh>
//#include <seastar/core/seastar.hh>
//#include <seastar/core/thread.hh>
//#include <seastar/core/reactor.hh>

//#include <seastar/net/api.hh>
//#include <seastar/net/tcp.hh>
//#include <seastar/net/stack.hh>

#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>

#include <boost/program_options.hpp>

#include <yaml-cpp/yaml.h>

namespace memoria {
namespace tests {

//namespace ss = seastar;

namespace net = boost::asio;

class NOOPConfigurator: public TestConfigurator {
    YAML::Node configuration_;
    fs::path config_base_path_;
public:

    NOOPConfigurator(YAML::Node configuration, fs::path config_base_path):
        configuration_(configuration),
        config_base_path_(config_base_path)
    {}

    YAML::Node& configuration() {
        return configuration_;
    }

    fs::path config_base_path() const {
        return config_base_path_;
    }
};


class DefaultTestContext: public TestContext {
    NOOPConfigurator configurator_;
    fs::path data_directory_;

    TestStatus status_{TestStatus::PASSED};

    std::exception_ptr ex_;

    TestCoverage coverage_;

    bool replay_;

    int64_t seed_;

public:
    DefaultTestContext(
            YAML::Node configuration,
            fs::path config_base_path,
            fs::path data_directory,
            TestCoverage coverage,
            bool replay,
            int64_t seed
    ):
        configurator_(configuration, config_base_path),
        data_directory_(data_directory),
        coverage_(coverage),
        replay_(replay),
        seed_(seed)
    {}

    virtual TestCoverage coverage() const noexcept {return coverage_;}
    TestStatus status() const noexcept {return status_;}

    std::exception_ptr& ex() {return ex_;}

    virtual TestConfigurator* configurator() noexcept {
        return &configurator_;
    }

    virtual std::ostream& out() noexcept {
        return std::cout;
    }

    virtual fs::path data_directory() noexcept {
        return data_directory_;
    }

    virtual void passed() noexcept
    {
        status_ = TestStatus::PASSED;
    }

    virtual void failed(TestStatus detail, std::exception_ptr ex, TestState* state) noexcept;

    virtual bool is_replay() const noexcept {
        return replay_;
    }

    int64_t seed() const noexcept {
        return seed_;
    }
};

void dump_exception(std::ostream& out, std::exception_ptr& ex);
TestStatus run_single_test(const U8String& test_path);
TestStatus replay_single_test(const U8String& test_path);


void run_tests2(net::io_service& ios, size_t threads);


class AbstractWorker {
protected:
    U8String server_address_;
    uint16_t port_;
    size_t worker_num_;
    fs::path output_folder_;
public:
    AbstractWorker(U8String address, uint16_t port, size_t worker_num, fs::path output_folder):
        server_address_(address),
        port_(port),
        worker_num_(worker_num),
        output_folder_(output_folder)
    {
    }

    virtual void handle_messages();

    virtual bool write_message(U8String msg) = 0;
    virtual hermes::HermesCtr read_message() = 0;

    virtual void run() = 0;
};


class ASIOWorker: public AbstractWorker {
    net::io_service& ios_;
    net::ip::tcp::socket socket_;


public:
    ASIOWorker(net::io_service& ios, U8String address, uint16_t port, size_t worker_num, fs::path output_folder):
        AbstractWorker(address, port, worker_num, output_folder),
        ios_(ios),
        socket_(ios)
    {
    }

    void run() override;

    bool write_message(U8String msg) override;
    hermes::HermesCtr read_message() override;

    bool write_data(const void* data, size_t size);
    size_t read_data(uint8_t* data, size_t size);
    bool read_data_fully(void* data, size_t size);
};



enum class Status {
    RUNNING, EXITED, TERMINATED, CRASHED, FAILED, UNKNOWN
};

std::ostream& operator<<(std::ostream& out, Status status);

class Process {
    boost::process::child process_;
public:
    Process(
        std::string filename,
        const std::vector<std::string>& args,
        std::string stdout_fname,
        std::string stderr_fname
    );

    Process(Process&& process):
        process_(std::move(process.process_))
    {}

    Process(const Process& process) = delete;

    bool valid() {
        return process_.valid();
    }

    Status join();

    int32_t exit_code() const {
        return process_.exit_code();
    }

    void detach() {
        process_.detach();
    }

    bool joinable() {
        return process_.joinable();
    }

    void kill() {
        process_.terminate();
    }

    void terminate();
};

class WorkerProcess: public std::enable_shared_from_this<WorkerProcess> {
    std::unique_ptr<Process> process_;

    U8String socket_addr_;
    uint16_t port_;
    fs::path output_folder_;
    size_t worker_num_;
public:

    using StatusListener = std::function<void (Status, int32_t)>;
private:

    StatusListener status_listener_;

public:
    WorkerProcess(U8String socket_addr, uint16_t port, fs::path output_folder, size_t num):
        socket_addr_(socket_addr),
        port_(port),
        output_folder_(output_folder),
        worker_num_(num)
    {}

    ~WorkerProcess() noexcept;

    void set_status_listener(const StatusListener& ll) {
        status_listener_ = ll;
    }

    boost::fibers::future<void> start();

    Status join() {
        return process_->join();
    }

    void terminate() {
        process_->terminate();
    }

    void kill() {
        process_->terminate();
    }
};


class MultiProcessRunner: public std::enable_shared_from_this<MultiProcessRunner> {
    net::io_service& ios_;
    net::ip::tcp::socket socket_;
    std::vector<std::shared_ptr<WorkerProcess>> worker_processes_;
    size_t workers_num_;
    U8String address_;
    uint16_t port_;

    size_t crashes_{};
    std::map<size_t, U8String> heads_;

    std::list<std::pair<U8String, boost::fibers::future<void>>> threads_;

    bool respawn_workers_{true};

    std::list<U8String> tests_;

public:
    MultiProcessRunner(net::io_service& ios, size_t workers_num, const U8String& address = "0.0.0.0", uint16_t port = 0):
        ios_(ios),
        socket_(ios),
        workers_num_(workers_num),
        address_(address),
        port_(port)
    {}

    void start();

private:
    std::shared_ptr<WorkerProcess> create_worker(size_t num);

    void handle_connections();

    void ping_socket();
};

boost::program_options::variables_map& get_config();

fs::path get_program_path();
fs::path get_image_name();

}}
