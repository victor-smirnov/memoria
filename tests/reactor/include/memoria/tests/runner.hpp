
// Copyright 2018-2021 Victor Smirnov
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
#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/process.hpp>
#include <memoria/reactor/socket.hpp>

#include <yaml-cpp/yaml.h>

namespace memoria {
namespace tests {

class NOOPConfigurator: public TestConfigurator {
    YAML::Node configuration_;
    boost::filesystem::path config_base_path_;
public:

    NOOPConfigurator(YAML::Node configuration, boost::filesystem::path config_base_path):
        configuration_(configuration),
        config_base_path_(config_base_path)
    {}

    YAML::Node& configuration() {
        return configuration_;
    }

    boost::filesystem::path config_base_path() const {
        return config_base_path_;
    }
};


class DefaultTestContext: public TestContext {
    NOOPConfigurator configurator_;
    boost::filesystem::path data_directory_;

    TestStatus status_{TestStatus::PASSED};

    std::exception_ptr ex_;

    TestCoverage coverage_;

    bool replay_;

    int64_t seed_;

public:
    DefaultTestContext(
            YAML::Node configuration,
            boost::filesystem::path config_base_path,
            boost::filesystem::path data_directory,
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
        return reactor::engine().cout();
    }

    virtual boost::filesystem::path data_directory() noexcept {
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

void run_tests();
void run_tests2(size_t threads);

class Worker {
    reactor::ClientSocket socket_;
    reactor::IPAddress server_address_;
    uint16_t port_;
    size_t worker_num_;
    boost::filesystem::path output_folder_;
public:
    Worker(reactor::IPAddress server_address, uint16_t port, size_t worker_num, boost::filesystem::path output_folder):
        server_address_(server_address),
        port_(port),
        worker_num_(worker_num),
        output_folder_(output_folder)
    {}

    void run();
};

class WorkerProcess: public std::enable_shared_from_this<WorkerProcess> {
    reactor::Process process_;

    reactor::File out_file_;
    reactor::File err_file_;
    std::unique_ptr<reactor::InputStreamReaderWriter> out_reader_;
    std::unique_ptr<reactor::InputStreamReaderWriter> err_reader_;

    reactor::IPAddress socket_addr_;
    uint16_t port_;
    boost::filesystem::path output_folder_;
    size_t worker_num_;
public:

    using StatusListener = std::function<void (reactor::Process::Status, int32_t)>;
private:

    StatusListener status_listener_;

public:
    WorkerProcess(reactor::IPAddress socket_addr, uint16_t port, boost::filesystem::path output_folder, size_t num):
        socket_addr_(socket_addr),
        port_(port),
        output_folder_(output_folder),
        worker_num_(num)
    {}

    ~WorkerProcess() noexcept;

    void set_status_listener(const StatusListener& ll) {
        status_listener_ = ll;
    }

    reactor::Process::Status status() const;

    boost::fibers::fiber start();

    void join() {
        process_.join();
    }

    void terminate() {
        process_.terminate();
    }

    void kill() {
        process_.kill();
    }
};


class MultiProcessRunner: public std::enable_shared_from_this<MultiProcessRunner> {
    reactor::ServerSocket socket_;
    std::vector<std::shared_ptr<WorkerProcess>> worker_processes_;
    size_t workers_num_;
    reactor::IPAddress address_;
    uint16_t port_;

    size_t crashes_{};
    std::map<size_t, U8String> heads_;

    std::list<std::pair<U8String, boost::fibers::fiber>> fibers_;

    bool respawn_workers_{true};

public:
    MultiProcessRunner(size_t workers_num, const reactor::IPAddress& address = reactor::IPAddress(), uint16_t port = 0):
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

}}
