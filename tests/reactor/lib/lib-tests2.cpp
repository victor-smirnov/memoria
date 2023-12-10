
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



#include <memoria/tests/tests.hpp>
#include <memoria/tests/arg_helper.hpp>

#include <memoria/reactor/reactor.hpp>

namespace memoria {
namespace tests {

Test* TestSuite::find(const U8String &name)
{
    auto ii = tests_.find(name);
    if (ii != tests_.end())
    {
        return ii->second.get();
    }
    else {
        return nullptr;
    }
}


TestSuite& TestsRegistry::get_suite(const U8String& name)
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

TestSuite* TestsRegistry::find_suite(const U8String& suite_name)
{
    auto ii = suites_.find(suite_name);
    if (ii != suites_.end())
    {
        return ii->second.get();
    }
    else {
        return nullptr;
    }
}


std::tuple<U8String, U8String> TestsRegistry::split_path(U8String test_path)
{
    U8String suite_name;
    U8String test_name;

    size_t delimiter_pos = test_path.find_first_of("/");
    if (delimiter_pos != U8String::NPOS) {
        suite_name = test_path.substring(0, delimiter_pos).trim();
        test_name  = test_path.substring(delimiter_pos + 1).trim();
    }
    else {
        suite_name = DEFAULT_SUITE_NAME;
        test_name  = test_path.trim_copy();
    }

    return std::make_tuple(suite_name, test_name);
}

Test* TestsRegistry::find_test(const U8String& test_path)
{
    U8String suite_name;
    U8String test_name;

    std::tie(suite_name, test_name) = split_path(test_path);

    auto suite = find_suite(suite_name);

    if (suite) {
        return suite->find(test_name);
    }
    else {
        return nullptr;
    }
}

void TestsRegistry::print() const
{
    for (auto& suite: suites_) {
        for (auto& test: suite.second->tests()) {
            reactor::engine().coutln("{}/{}", suite.first, test.first);
        }
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
                args_.insert(args_.begin() + 1, make_copy("-t"));
                args_.insert(args_.begin() + 2, make_copy("1"));
            }
        }
    }
}


}}
