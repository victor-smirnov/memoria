
// Copyright 2019-2025 Victor Smirnov
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

auto ldd_map_add_remove_tests = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "MapSetRemove", [](auto& state){
    auto doc = hermes::HermesCtrView::make_new();

    auto map = doc.make_object_map();
    assert_equals(0, map.size());

    size_t size = 10000;

    std::unordered_map<U8String, int64_t> values;
    for (size_t c = 0; c < size; c++)
    {
        U8String key = "Entry" + std::to_string(c);
        values[key] = c;
        map.put_t<BigInt>(key, c);

        if (c % 500 == 0) {
            assert_arrays_equal(values, map);
        }
    }
    assert_arrays_equal(values, map);

    for (size_t c = 0; c < size; c++)
    {
        U8String key = "Entry" + std::to_string(c);

        map.remove(key);
        values.erase(key);

        if (c % 500 == 0) {
            assert_arrays_equal(values, map);
        }
    }
    assert_equals(0, map.size());
});


auto ldd_map_set_tests = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "MapSet", [](auto& state){
    auto doc = hermes::HermesCtrView::make_new();

    auto map = doc.make_object_map();

    assert_equals(0, map.size());

    map.put("Entry0", "Hello World");
    assert_equals(true, map.expect("Entry0").is_varchar());
    assert_equals("Hello World", map.expect("Entry0").as_varchar());

    map.put_t<Double>("Entry1", 123456);
    assert_equals(true, map.expect("Entry1").is_double());
    assert_equals(123456, map.expect("Entry1").as_double());

    map.put("Entry2", true);
    assert_equals(true, map.expect("Entry2").is_boolean());
    assert_equals(true, map.expect("Entry2").as_boolean());
});

}}
