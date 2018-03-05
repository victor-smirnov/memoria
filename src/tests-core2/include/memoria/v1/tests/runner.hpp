
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/reactor/reactor.hpp>

namespace memoria {
namespace v1 {
namespace tests {

struct NOOPConfigurator: public TestConfigurator {

};


class DefaultTestContext: public TestContext {
    NOOPConfigurator configurator_;
    filesystem::path data_directory_;

    TestStatus status_{TestStatus::PASSED};

    std::exception_ptr ex_;

public:

    TestStatus status() const {return status_;}

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

    virtual void failed(TestStatus detail, std::exception_ptr ex) noexcept {
        status_ = detail;
        ex_ = ex;
    }
};

void dump_exception(std::ostream& out, std::exception_ptr& ex);
TestStatus run_single_test(const U16String& test_path);

void run_tests();

}}}
