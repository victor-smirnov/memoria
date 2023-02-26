
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/strings/format.hpp>

#include <ostream>

namespace memoria {

[[clang::annotation(R"(
    @Test = {
        'suite': 'CoreTests'
    }
)")]]
void do_simple_test();

struct ReplayableTestParams {
    bool replay{};
};



struct ParametricTestParams {
    int a{1};
    int b{2};
    long c{3};
};


[[clang::annotation(R"(
    @Test = {
        'suite': 'CoreTests'
    }
)")]]
void do_simple_parametric_test(ParametricTestParams& params);

#define MMA_TEST [[clang::annotation(R"(@Test = {})")]]
#define MMA_TEST_SUITE [[clang::annotation(R"(@TestSuite = {})")]]

struct TestContext {
    virtual ~TestContext() = default;
    std::ostream& out();

    template <typename... Args>
    void println(const char* fmt, Args&&... args) {
        ::memoria::println(out(), fmt, std::forward<Args>(args)...);
    }
};

TestContext& ctx();

struct MMA_TEST_SUITE SimpleTestSuite {

    MMA_TEST
    void do_simple_thing() {
        ctx().println("{}", "Hello!");
    }

};


template <
    typename T1
>
struct [[clang::annotation(R"(
    'instantiations': {
        'Simple': 'SuiteTemplate<int>'
    }
)")]] SuiteTemplate {


};


}
