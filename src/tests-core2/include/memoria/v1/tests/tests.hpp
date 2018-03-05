
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/strings/format.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/filesystem/path.hpp>
#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/tools/ptr_cast.hpp>

#include <memoria/v1/core/tools/optional.hpp>

#include <memory>
#include <vector>
#include <functional>
#include <ostream>
#include <unordered_map>
#include <exception>

namespace memoria {
namespace v1 {
namespace tests {

struct TestException: virtual Exception {};
struct TestConfigurationException: virtual TestException {};
struct TestExecutionException: virtual TestException {};

enum class TestStatus {PASSED, TEST_FAILED, SETUP_FAILED};

struct TestConfigurator {
    virtual ~TestConfigurator() noexcept {}
};

struct TestContext {
    virtual ~TestContext() noexcept {}

    virtual TestConfigurator* configurator() noexcept  = 0;
    virtual std::ostream& out() noexcept               = 0;
    virtual filesystem::path data_directory() noexcept = 0;

    virtual void passed() noexcept                                          = 0;
    virtual void failed(TestStatus detail, std::exception_ptr ex) noexcept  = 0;
};





struct TestState {

    TestState() {}
    TestState(TestState&&) = default;
    TestState(const TestState&) = delete;

    virtual ~TestState() noexcept {}

};


struct Test {
    virtual ~Test() {}
    virtual std::unique_ptr<TestState> create_state(TestConfigurator* configurator) = 0;
    void run(TestContext* context) noexcept;

    virtual void test(TestState* state) = 0;
};

struct TestSuite {
    using TestsMap = std::map<U16String, std::unique_ptr<Test>>;
private:
    TestsMap tests_;
public:
    TestSuite(const TestSuite&) = delete;
    TestSuite() = default;

    const TestsMap& tests() const {
        return tests_;
    }

    template <typename TestT, typename... Args>
    void emplace(U16String test_name, Args&&... args)
    {
        auto ii = tests_.find(test_name);
        if (ii == tests_.end())
        {
            tests_[test_name] = std::make_unique<TestT>(std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(TestConfigurationException()) << fmt::format_ex(u"Test {} has been already registered", test_name);
        }
    }

    Optional<Test&> find(const U16String& name);
};



struct TestsRegistry {

    using SuitesMap = std::map<U16String, std::unique_ptr<TestSuite>>;

private:
    SuitesMap suites_;
public:

    static constexpr const char16_t* DEFAULT_SUITE_NAME = u"QuickTests";

    TestsRegistry() {}
    TestsRegistry(const TestsRegistry&) = delete;
    TestsRegistry(TestsRegistry&&) = delete;

    ~TestsRegistry() noexcept {}

    const SuitesMap& suites() const {
        return suites_;
    }

    void configure(TestContext* context);

    TestSuite& get_suite(const U16String& name);

    Optional<TestSuite&> find_suite(const U16String& suite_name);
    Optional<Test&> find_test(const U16String& test_path);

    template <typename TestT, typename... Args>
    void emplace_in_suite(const U16String& suite_name, const U16String& test_name, Args&&... args)
    {
        auto& suite = get_suite(suite_name);
        suite.emplace<TestT>(test_name, std::forward<Args>(args)...);
    }

    template <typename TestT, typename... Args>
    void emplace(const U16String& test_name, Args&&... args)
    {
        auto& suite = get_suite(DEFAULT_SUITE_NAME);
        suite.emplace<TestT>(test_name, std::forward<Args>(args)...);
    }
};

TestsRegistry& tests_registry();


template <typename TestT, typename... Args>
EmptyType register_test(const U16String& test_name, Args&&... args)
{
    tests_registry().emplace<TestT>(test_name, std::forward<Args>(args)...);
    return EmptyType();
}

template <typename TestT, typename... Args>
EmptyType register_test_in_suite(const U16String& suite_name, const U16String& test_name, Args&&... args)
{
    tests_registry().emplace_in_suite<TestT>(test_name, std::forward<Args>(args)...);
    return EmptyType();
}

struct EmptyState: TestState{
    EmptyState(TestConfigurator* configurator) {}
};

template <typename StateT = EmptyState>
class FnTest: public Test {
    std::function<void(StateT&)> fn_;
public:
    template <typename FnT>
    FnTest(FnT&& fn): fn_(std::move(fn)) {}

    void test(TestState* state) {
        fn_(*tools::ptr_cast<StateT>(state));
    }

    std::unique_ptr<TestState> create_state(TestConfigurator* configurator) {
        return std::make_unique<StateT>(configurator);
    }
};


}}}
