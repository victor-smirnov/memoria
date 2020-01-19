
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

#pragma once

#include <memoria/tests/tests.hpp>
#include <memoria/reactor/reactor.hpp>

#include <yaml-cpp/yaml.h>

namespace memoria {
namespace tests {

class NOOPConfigurator: public TestConfigurator {
    YAML::Node configuration_;
    filesystem::path config_base_path_;
public:

    NOOPConfigurator(YAML::Node configuration, filesystem::path config_base_path):
        configuration_(configuration),
        config_base_path_(config_base_path)
    {}

    YAML::Node& configuration() {
        return configuration_;
    }

    filesystem::path config_base_path() const {
        return config_base_path_;
    }
};


class DefaultTestContext: public TestContext {
    NOOPConfigurator configurator_;
    filesystem::path data_directory_;

    TestStatus status_{TestStatus::PASSED};

    std::exception_ptr ex_;

    TestCoverage coverage_;

    bool replay_;

    int64_t seed_;

public:
    DefaultTestContext(
            YAML::Node configuration,
            filesystem::path config_base_path,
            filesystem::path data_directory,
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

    virtual filesystem::path data_directory() noexcept {
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

}}
