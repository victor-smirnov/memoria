
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

#include <memoria/v1/tests/tests.hpp>

namespace memoria {
namespace v1 {
namespace tests {

template <typename T1, typename T2>
void assert_equals(T1&& expected, T2&& actual) {
    if (!(expected == actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"Expected {}, actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_equals(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected == actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"{}: Expected {}, actual {}", msg, expected, actual);
    }
}


template <typename T1, typename T2, typename... Args>
void assert_equals(T1&& expected, T2&& actual, const char16_t* msg, Args&&... args) {
    if (!(expected == actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"Equality: Expected {}, actual {}, detail: {}", expected, actual, fmt::format(msg, std::forward<Args>(args)...));
    }
}



template <typename T1, typename T2>
void assert_neq(T1&& expected, T2&& actual) {
    if (expected == actual) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"Value {} is equals to actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_neq(T1&& expected, T2&& actual, const char* msg) {
    if (expected == actual) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"{}: Value {} is equals to actual {}", msg, expected, actual);
    }
}




template <typename T1, typename T2>
void assert_gt(T1&& expected, T2&& actual) {
    if (!(expected > actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"Expected {} is not greather than actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_gt(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected > actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"{}: Expected {} is not greather than actual {}", msg, expected, actual);
    }
}



template <typename T1, typename T2>
void assert_lt(T1&& expected, T2&& actual) {
    if (!(expected < actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"Expected {} is not less than actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_lt(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected < actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"{}: Expected {} is not less than actual {}", msg, expected, actual);
    }
}




template <typename T1, typename T2>
void assert_gte(T1&& expected, T2&& actual) {
    if (!(expected >= actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"Expected {} is not greather than or equal to actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_gte(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected >= actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"{}: Expected {} is not greather than or equal to actual {}", msg, expected, actual);
    }
}



template <typename T1, typename T2>
void assert_lte(T1&& expected, T2&& actual) {
    if (!(expected <= actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"Expected {} is not less than or equal to actual {}", expected, actual);
    }
}


template <typename T1, typename T2>
void assert_lte(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected <= actual)) {
        MMA1_THROW(TestExecutionException()) << fmt::format_ex(u"{}: Expected {} is not less than or equal to actual {}", msg, expected, actual);
    }
}

}}}
