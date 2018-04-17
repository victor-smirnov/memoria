
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
#include <memoria/v1/tests/arg_helper.hpp>

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



char* ThreadsArgHelper::make_copy(const char* str)
{
    char* copy = allocate_system<char>(::strlen(str) + 1).release();
    ::strcpy(copy, str);
    return copy;
}

char* ThreadsArgHelper::has_arg(const std::string& target_arg_name)
{
    size_t max = args_.size() - 1;
    for (size_t c = 0; c < max; c++)
    {
        if (target_arg_name == args_[c])
        {
            if (c < max - 1){
                return args_[c + 1];
            }
            else {
                std::cout << "Incorrect " << target_arg_name << " command line switch. Aborting." << std::endl;
                std::terminate();
            }
        }
    }

    return nullptr;
}


void ThreadsArgHelper::fix_thread_arg()
{
    char* test_name = has_arg("--test");

    if (test_name)
    {
        char* threads1 = has_arg("--threads");
        char* threads2 = has_arg("-t");

        if (!(threads1 || threads2))
        {
            auto test = tests_registry().find_test(test_name);
            if (test)
            {
                //auto state = test.get().create_state();

                args_.insert(args_.begin() + 1, make_copy("-t"));
                args_.insert(args_.begin() + 2, make_copy("1"));

                //args_.insert(args_.begin() + 2, make_copy(std::to_string(state->threads()).data()));
            }
        }
    }
}


}}}
