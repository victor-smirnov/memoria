
// Copyright 2019 Victor Smirnov
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


#include <memoria/core/linked/document/linked_document.hpp>

#include "ld_test_tools.hpp"


namespace memoria {
namespace tests {

auto ldd_map_add_remove_tests = register_test_in_suite<FnTest<LDTestState>>("LDDocumentTestSuite", "MapSetRemove", [](auto& state){
    auto doc = LDDocument::make_new();

    auto map = doc->set_map();
    assert_equals(true, *doc->value()->as_map() == *map);
    assert_equals(0, map->size());

    size_t size = 10000;

    std::unordered_map<U8String, int64_t> values;
    for (size_t c = 0; c < size; c++)
    {
        U8String key = "Entry" + std::to_string(c);
        values[key] = c;
        map->set_bigint(key, c);

        if (c % 500 == 0) {
            assert_arrays_equal(values, *map);
        }
    }

    assert_arrays_equal(values, *map);


    for (size_t c = 0; c < size; c++)
    {
        U8String key = "Entry" + std::to_string(c);

        map->remove(key);
        values.erase(key);

        if (c % 500 == 0) {
            assert_arrays_equal(values, *map);
        }
    }

    assert_equals(0, map->size());
});


auto ldd_map_set_tests = register_test_in_suite<FnTest<LDTestState>>("LDDocumentTestSuite", "MapSet", [](auto& state){
    auto doc = LDDocument::make_new();

    auto map = doc->set_map();
    assert_equals(true, *doc->value()->as_map() == *map);
    assert_equals(0, map->size());

    map->set_varchar("Entry0", "Hello World");
    assert_equals(true, map->get("Entry0")->is_varchar());
    assert_equals("Hello World", *map->get("Entry0")->as_varchar()->view());

    map->set_double("Entry1", 123456);
    assert_equals(true, map->get("Entry1")->is_double());
    assert_equals(123456, *map->get("Entry1")->as_double());

    map->set_boolean("Entry2", true);
    assert_equals(true, map->get("Entry2")->is_boolean());
    assert_equals(true, *map->get("Entry2")->as_boolean());

    auto map1 = map->set_map("Entry3");
    assert_equals(true, map->get("Entry3")->is_map());
    assert_equals(true, *map->get("Entry3")->as_map() == *map1);

    auto arr = map->set_array("Entry4");
    assert_equals(true, map->get("Entry4")->is_array());
    assert_equals(true, *map->get("Entry4")->as_array() == *arr);

    auto sdn1 = map->set_sdn("Entry5", "'123456'@CoolType");
    assert_equals(true, map->get("Entry5")->is_typed_value());
    assert_equals(true, *map->get("Entry5") == *sdn1);

    auto sdn2 = map->set_sdn("Entry6", "CoolType");
    assert_equals(true, map->get("Entry6")->is_type_decl());
    assert_equals(true, *map->get("Entry6") == *sdn2);

    auto doc2 = map->set_document("Entry7", "{}");
    assert_equals(true, map->get("Entry7")->is_map());
    assert_equals(true, *map->get("Entry7") == *doc2);

    map->set_null("Entry8");
    assert_equals(true, map->get("Entry8")->is_null());
});

}}
