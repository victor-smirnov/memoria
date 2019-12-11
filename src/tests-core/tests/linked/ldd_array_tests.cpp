
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

#include <memoria/v1/core/linked/document/linked_document.hpp>

#include "ld_test_tools.hpp"

namespace memoria {
namespace v1 {
namespace tests {


auto ldd_array_add_remove_tests = register_test_in_suite<FnTest<LDTestState>>(u"LDDocumentTestSuite", u"ArrayAddRemove", [](auto& state){
    LDDocument doc;

    LDDArrayView array = doc.set_array();
    assert_equals(true, doc.value().as_array() == array);
    assert_equals(0, array.size());

    size_t size = 10000;

    std::vector<int64_t> values;
    for (size_t c = 0; c < size; c++)
    {
        array.add_integer(12345 + c);
        values.push_back(12345 + c);
        if (c % 100 == 0) {
            assert_arrays_equal(values, array);
        }
    }

    assert_arrays_equal(values, array);

    assert_throws<BoundsException>([&](){
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


auto ldd_array_set_tests = register_test_in_suite<FnTest<LDTestState>>(u"LDDocumentTestSuite", u"ArraySet", [](auto& state){
    LDDocument doc;

    LDDArrayView array = doc.set_array();
    assert_equals(true, doc.value().as_array() == array);
    assert_equals(0, array.size());

    array.add_string("Hello World");
    assert_equals(true, array.get(0).is_varchar());
    assert_equals("Hello World", array.get(0).as_varchar().view());

    array.add_double(123456);
    assert_equals(true, array.get(1).is_double());
    assert_equals(123456, array.get(1).as_double());

    array.add_boolean(true);
    assert_equals(true, array.get(2).is_boolean());
    assert_equals(true, array.get(2).as_boolean());

    LDDMapView map = array.add_map();
    assert_equals(true, array.get(3).is_map());
    assert_equals(true, array.get(3).as_map() == map);

    LDDArrayView arr = array.add_array();
    assert_equals(true, array.get(4).is_array());
    assert_equals(true, array.get(4).as_array() == arr);

    LDDValueView sdn1 = array.add_sdn("'123456'@CoolType");
    assert_equals(true, array.get(5).is_typed_value());
    assert_equals(true, array.get(5) == sdn1);

    LDDValueView sdn2 = array.add_sdn("CoolType");
    assert_equals(true, array.get(6).is_type_decl());
    assert_equals(true, array.get(6) == sdn2);

    LDDValueView doc2 = array.add_document("{}");
    assert_equals(true, array.get(7).is_map());
    assert_equals(true, array.get(7) == doc2);

    array.add_null();
    assert_equals(true, array.get(8).is_null());


    array.set_double(0, 555);
    assert_equals(true, array.get(0).is_double());
    assert_equals(555, array.get(0).as_double());

    assert_throws<BoundsException>([&](){
        array.set_double(10, 555);
    });

    array.set_bigint(1, 555);
    assert_equals(true, array.get(1).is_bigint());
    assert_equals(555, array.get(1).as_bigint());

    assert_throws<BoundsException>([&](){
        array.set_bigint(10, 555);
    });

    array.set_boolean(2, false);
    assert_equals(true, array.get(2).is_boolean());
    assert_equals(false, array.get(2).as_boolean());

    assert_throws<BoundsException>([&](){
        array.set_boolean(10, false);
    });

    array.set_varchar(3, "Cool String");
    assert_equals(true, array.get(3).is_varchar());
    assert_equals("Cool String", array.get(3).as_varchar().view());

    assert_throws<BoundsException>([&](){
        array.set_varchar(10, "S0");
    });


    array.set_sdn(4, "{}");
    assert_equals(true, array.get(4).is_map());
    assert_throws<BoundsException>([&](){
        array.set_sdn(10, "S0");
    });

    array.set_document(5, "[]");
    assert_equals(true, array.get(5).is_array());
    assert_throws<BoundsException>([&](){
        array.set_document(10, "S0");
    });

});

}}}
