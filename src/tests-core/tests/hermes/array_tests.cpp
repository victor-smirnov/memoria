
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


auto ldd_array_add_remove_tests = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "ArrayAddRemove", [](auto& state){
    auto doc = hermes::HermesCtr::make_new();

    auto array = doc->new_array();
    doc->set_root(array->as_object());
    //assert_equals(true, doc->root()->as_generic_array()->equals(array));
    assert_equals(0, array->size());

    size_t size = 10000;

    std::vector<int64_t> values;
    for (size_t c = 0; c < size; c++)
    {
        array->append<BigInt>(12345 + c);
        values.push_back(12345 + c);
        if (c % 100 == 0) {
            assert_arrays_equal(values, *array);
        }
    }

    assert_arrays_equal(values, *array);

    assert_throws<ResultException>([&](){
        array->get(size);
    });

    for (size_t c = 0; c < size; c++) {
        array->remove(0);
        values.erase(values.begin(), values.begin() + 1);

        if (c % 100 == 0) {
            assert_arrays_equal(values, *array);
        }
    }

    assert_equals(0, array->size());
});


auto ldd_array_set_tests = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "ArraySet", [](auto& state){
    auto doc = hermes::HermesCtr::make_new();

    auto array = doc->new_array();
    doc->set_root(array->as_object());
//    assert_equals(true, doc->root()->as_generic_array()->equals(array));
    assert_equals(0, array->size());

    array->append<Varchar>("Hello World");
    assert_equals(true, array->get(0)->is_varchar());
    assert_equals("Hello World", array->get(0)->as_varchar()->view());

    array->append<Double>(123456);
    assert_equals(true, array->get(1)->is_double());
    assert_equals(123456, array->get(1)->as_double()->view());

    array->append<Boolean>(true);
    assert_equals(true, array->get(2)->is_boolean());
    assert_equals(true, array->get(2)->as_boolean()->view());

    auto map = doc->new_map();
    array->append(map->as_object());
    assert_equals(true, array->get(3)->is_map());
//    assert_equals(true, array->get(3)->as_generic_map()->equals(map));

    auto arr = doc->new_array();
    array->append(arr->as_object());
    assert_equals(true, array->get(4)->is_array());
//    assert_equals(true, array->get(4)->as_generic_array()->equals(arr));

    //auto sdn1 = array->append_hermes("'123456'@CoolType");
    //assert_equals(true, array->get(5)->is_typed_value());
    //assert_equals(true, array->get(5)->equals(sdn1));

    //auto sdn2 = array->append_hermes("CoolType");
    //assert_equals(true, array->get(6)->is_datatype());
    //assert_equals(true, array->get(6)->equals(sdn2));

    //auto doc2 = array->append_hermes("{}");
    //assert_equals(true, array->get(7)->is_map());
    //assert_equals(true, array->get(7)->equals(doc2));

    //array->append_null();
    //assert_equals(true, array->get(8)->is_null());


    array->set<Double>(0, 555);
    assert_equals(true, array->get(0)->is_double());
    assert_equals(555, array->get(0)->as_double()->view());

    assert_throws<ResultException>([&](){
        array->set<Double>(10, 555);
    });

    array->set<BigInt>(1, 555);
    assert_equals(true, array->get(1)->is_bigint());
    assert_equals(555, array->get(1)->as_bigint()->view());

    assert_throws<ResultException>([&](){
        array->set<BigInt>(10, 555);
    });

    array->set<Boolean>(2, false);
    assert_equals(true, array->get(2)->is_boolean());
    assert_equals(false, array->get(2)->as_boolean()->view());

    assert_throws<ResultException>([&](){
        array->set<Boolean>(10, false);
    });

    array->set<Varchar>(3, "Cool String");
    assert_equals(true, array->get(3)->is_varchar());
    assert_equals("Cool String", array->get(3)->as_varchar()->view());

    assert_throws<ResultException>([&](){
        array->set<Varchar>(10, "S0");
    });


    array->set_hermes(4, "{}");
    assert_equals(true, array->get(4)->is_map());
    assert_throws<ResultException>([&](){
        array->set_hermes(10, "S0");
    });

    array->set_hermes(5, "[]");
    assert_equals(true, array->get(5)->is_array());
    assert_throws<ResultException>([&](){
        array->set_hermes(10, "S0");
    });

});

}}
