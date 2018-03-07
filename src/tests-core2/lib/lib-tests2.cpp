
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



#include <memoria/v1/tests/tests.hpp>

namespace memoria {
namespace v1 {
namespace tests {

Optional<Test&> TestSuite::find(const U16String &name)
{
    auto ii = tests_.find(name);
    if (ii != tests_.end())
    {
        return *ii->second.get();
    }
    else {
        return Optional<Test&>();
    }
}


TestSuite& TestsRegistry::get_suite(const U16String& name)
{
    auto ii = suites_.find(name);
    if (ii != suites_.end())
    {
        return *ii->second.get();
    }
    else {
        auto suite = std::make_unique<TestSuite>();
        auto suite_ptr = suite.get();

        suites_[name] = std::move(suite);

        return *suite_ptr;
    }
}

Optional<TestSuite&> TestsRegistry::find_suite(const U16String& suite_name)
{
    auto ii = suites_.find(suite_name);
    if (ii != suites_.end())
    {
        return *ii->second.get();
    }
    else {
        return Optional<TestSuite&>();
    }
}


std::tuple<U16String, U16String> TestsRegistry::split_path(U16String test_path)
{
    U16String suite_name;
    U16String test_name;

    size_t delimiter_pos = test_path.find_first_of(u"/");
    if (delimiter_pos != U16String::NPOS)
    {
        suite_name = test_path.substring(0, delimiter_pos).trim();
        test_name  = test_path.substring(delimiter_pos + 1).trim();
    }
    else {
        suite_name = DEFAULT_SUITE_NAME;
        test_name  = test_path.trim_copy();
    }

    return std::make_tuple(suite_name, test_name);
}

Optional<Test&> TestsRegistry::find_test(const U16String& test_path)
{
    U16String suite_name;
    U16String test_name;

    std::tie(suite_name, test_name) = split_path(test_path);

    auto suite = find_suite(suite_name);

    if (suite)
    {
        return suite->find(test_name);
    }
    else {
        return Optional<Test&>();
    }
}


TestsRegistry& tests_registry() {
    static TestsRegistry registry;
    return registry;
}


void Test::run(TestContext *context) noexcept
{
    std::unique_ptr<TestState> state;

    try {
        state = create_state(context->configurator());
        state->add_field_handlers();
        state->add_indirect_field_handlers();
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
