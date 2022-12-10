
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <memoria/tests/tests.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/span.hpp>



namespace memoria {
namespace tests {

template <typename T1, typename T2>
void assert_equals(T1&& expected, T2&& actual) {
    if (!(expected == actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("Expected {}, actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_equals(T1&& expected, Own<T2> actual) {
    if (!(expected == *actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("Expected {}, actual {}", expected, *actual);
    }
}

template <typename T1, typename T2>
void assert_equals(Span<T1> expected, Span<T2> actual) {
    if (expected.size() != actual.size()) {
        MMA_THROW(TestExecutionException()) << format_ex("Expected.size {}, actual.size {}", expected.size(), actual.size());
    }

    for (size_t c = 0; c < expected.size(); c++) {
        if (!(expected[c] == actual[c])) {
            MMA_THROW(TestExecutionException()) << format_ex("Expected {}, actual {} at {}", expected[c], actual[c], c);
        }
    }
}

template <typename T1, typename T2>
void assert_equals(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected == actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("{}: Expected {}, actual {}", msg, expected, actual);
    }
}


template <typename T1, typename T2, typename... Args>
void assert_equals(T1&& expected, T2&& actual, const char* msg, Args&&... args) {
    if (!(expected == actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("Equality: Expected {}, actual {}, detail: {}", expected, actual, fmt::format(msg, std::forward<Args>(args)...));
    }
}



template <typename T1, typename T2>
void assert_neq(T1&& expected, T2&& actual) {
    if (expected == actual) {
        MMA_THROW(TestExecutionException()) << format_ex("Value {} is equals to actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_neq(T1&& expected, T2&& actual, const char* msg) {
    if (expected == actual) {
        MMA_THROW(TestExecutionException()) << format_ex("{}: Value {} is equals to actual {}", msg, expected, actual);
    }
}




template <typename T1, typename T2>
void assert_gt(T1&& expected, T2&& actual) {
    if (!(expected > actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("Expected {} is not greather than actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_gt(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected > actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("{}: Expected {} is not greather than actual {}", msg, expected, actual);
    }
}



template <typename T1, typename T2>
void assert_lt(T1&& expected, T2&& actual) {
    if (!(expected < actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("Expected {} is not less than actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_lt(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected < actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("{}: Expected {} is not less than actual {}", msg, expected, actual);
    }
}




template <typename T1, typename T2>
void assert_ge(T1&& expected, T2&& actual) {
    if (!(expected >= actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("Expected {} is not greather than or equal to actual {}", expected, actual);
    }
}

template <typename T1, typename T2>
void assert_ge(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected >= actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("{}: Expected {} is not greather than or equal to actual {}", msg, expected, actual);
    }
}



template <typename T1, typename T2>
void assert_le(T1&& expected, T2&& actual) {
    if (!(expected <= actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("Expected {} is not less than or equal to actual {}", expected, actual);
    }
}


template <typename T1, typename T2>
void assert_le(T1&& expected, T2&& actual, const char* msg) {
    if (!(expected <= actual)) {
        MMA_THROW(TestExecutionException()) << format_ex("{}: Expected {} is not less than or equal to actual {}", msg, expected, actual);
    }
}


template <typename ExT, typename Fn, typename... Args>
void assert_throws(Fn&& fn, Args&&... args) {
    try {
        fn(std::forward<Args>(args)...);
    }
    catch (ExT& ex) {
        return;
    }
    catch (...) {
        MMA_THROW(TestExecutionException()) << format_ex("Unexpected exception has been thrown");
    }

    MMA_THROW(TestExecutionException()) << format_ex("Expected exception {} hasn't been thrown", TypeNameFactory<ExT>::name());
}

template <typename Fn, typename... Args>
void assert_fails(Fn&& fn, Args&&... args)
{
    auto res = wrap_throwing([&](){
        return fn(std::forward<Args>(args)...);
    });

    if (res.is_ok()) {
        MMA_THROW(TestExecutionException()) << format_ex("No expected falure has happened");
    }
}



template <typename ExT, typename Fn, typename... Args>
void assert_no_throws(Fn&& fn, Args&&... args) {
    try {
        fn(std::forward<Args>(args)...);
    }
    catch (ExT& ex) {
        MMA_THROW(TestExecutionException()) << format_ex("Function throws an expected exception {} ", TypeNameFactory<ExT>::name());
    }
    catch (...) {
        MMA_THROW(TestExecutionException()) << format_ex("Function throws an unexpected exception");
    }
}

}}
