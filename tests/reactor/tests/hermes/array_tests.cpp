
// Copyright 2019-2022 Victor Smirnov
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

#include "test_tools.hpp"

namespace memoria {
namespace tests {

using namespace memoria::hermes;

auto hermes_object_array_add_remove_tests = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "ObjectArrayAddRemove", [](auto& state){
    auto doc = HermesCtrView::make_new();

    auto array = doc.make_t<Array<Object>>();

    assert_equals(0, array.size());

    size_t size = 10000;

    std::vector<int64_t> values;
    for (size_t c = 0; c < size; c++)
    {
        array.push_back_t<BigInt>(12345 + c);
        values.push_back(12345 + c);
        if (c % 100 == 0) {
            assert_arrays_equal(values, array);
        }
    }

    assert_arrays_equal(values, array);

    assert_throws<ResultException>([&](){
        array.get(size);
    });

    for (size_t c = 0; c < size; c++) {
        array.remove(0);
        values.erase(values.begin(), values.begin() + 1);

        if (c % 100 == 0) {
            assert_arrays_equal(values, array);
        }
    }

    assert_equals(0, array.size());
});

auto hermes_integer_array_add_remove_tests = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "IntegerArrayAddRemove", [](auto& state){
    auto doc = HermesCtrView::make_new();

    auto array = doc.make_t<Array<Integer>>();
    assert_equals(0, array.size());

    size_t size = 10000;

    std::vector<int64_t> values;
    for (size_t c = 0; c < size; c++)
    {
        array.push_back(12345 + c);
        values.push_back(12345 + c);
        if (c % 100 == 0) {
            assert_arrays_equal(values, array);
        }
    }

    assert_arrays_equal(values, array);

    assert_throws<ResultException>([&](){
        array.get(size);
    });

    for (size_t c = 0; c < size; c++) {
        array.remove(0);
        values.erase(values.begin(), values.begin() + 1);

        if (c % 100 == 0) {
            assert_arrays_equal(values, array);
        }
    }

    assert_equals(0, array.size());
});


auto ldd_array_set_tests = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "ObjectArraySet", [](auto& state){
    auto doc = hermes::HermesCtrView::make_new();

    auto array = doc.make_object_array();
    assert_equals(0, array.size());

    array.push_back_t<Varchar>("Hello World");
    assert_equals(true, array.get(0).is_varchar());
    assert_equals("Hello World", array.get(0).as_varchar());

    array.push_back_t<Double>(123456);
    assert_equals(true, array.get(1).is_double());
    assert_equals(123456, array.get(1).as_double());

    array.push_back_t<Boolean>(true);
    assert_equals(true, array.get(2).is_boolean());
    assert_equals(true, array.get(2).as_boolean());

    auto map = doc.make_object_map();
    array.push_back(map.as_object());
    assert_equals(true, array.get(3).is_map());

    auto arr = doc.make_object_array();
    array.push_back(arr.as_object());
    assert_equals(true, array.get(4).is_array());

    array.set_t<Double>(0, 555);
    assert_equals(true, array.get(0).is_double());
    assert_equals(555, array.get(0).as_double());

    assert_throws<ResultException>([&](){
        array.set_t<Double>(10, 555);
    });

    array.set_t<BigInt>(1, 555);
    assert_equals(true, array.get(1).is_bigint());
    assert_equals(555, array.get(1).as_bigint());

    assert_throws<ResultException>([&](){
        array.set_t<BigInt>(10, 555);
    });

    array.set_t<Boolean>(2, false);
    assert_equals(true, array.get(2).is_boolean());
    assert_equals(false, array.get(2).as_boolean());

    assert_throws<ResultException>([&](){
        array.set_t<Boolean>(10, false);
    });

    array.set_t<Varchar>(3, "Cool String");
    assert_equals(true, array.get(3).is_varchar());
    assert_equals("Cool String", array.get(3).as_varchar());

    assert_throws<ResultException>([&](){
        array.set_t<Varchar>(10, "S0");
    });
});

}}
