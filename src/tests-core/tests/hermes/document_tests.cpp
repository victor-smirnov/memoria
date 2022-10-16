
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

auto document_set_test = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "Document", [](auto& state){
    auto doc = hermes::DocView::make_new();

    doc->set_dataobject<Double>(12345.67);
    assert_equals(true, doc->value()->is_double());
    assert_equals(12345.67, doc->value()->as_double()->view());
    assert_throws<ResultException>([&](){
        doc->value()->as_bigint();
    });


    doc->set_dataobject<Varchar>("Hello world");
    assert_equals(true, doc->value()->is_varchar());
    assert_equals("Hello world", doc->value()->as_varchar()->view());

    assert_throws<ResultException>([&](){
        doc->value()->as_double();
    });

    doc->set_dataobject<BigInt>(123);
    assert_equals(true, doc->value()->is_bigint());
    assert_equals(123, doc->value()->as_bigint()->view());

    assert_throws<ResultException>([&](){
        doc->value()->as_double();
    });

    doc->set_dataobject<Boolean>(1);
    assert_equals(true, doc->value()->is_boolean());
    assert_equals(1, doc->value()->as_boolean()->view());

    assert_throws<ResultException>([&](){
        doc->value()->as_double();
    });

    doc->set_hermes("12345");
    assert_equals(true, doc->value()->is_bigint());
    assert_equals(12345, doc->value()->as_bigint()->view());

    doc->set_hermes("{}");
    assert_equals(true, doc->value()->is_generic_map());
    assert_equals(0, doc->value()->as_generic_map()->size());
    assert_throws<ResultException>([&](){
        doc->value()->as_double();
    });

    doc->set_hermes("[]");
    assert_equals(true, doc->value()->is_generic_array());
    assert_equals(0, doc->value()->as_generic_array()->size());
    assert_throws<ResultException>([&](){
        doc->value()->as_double();
    });

    doc->set_hermes("Decimal(1,2)");
    assert_equals(true, doc->value()->is_datatype());
    assert_throws<ResultException>([&](){
        doc->value()->as_double();
    });

    doc->set_null();
    assert_equals(true, doc->value()->is_null());
    assert_throws<ResultException>([&](){
        doc->value()->as_double();
    });

    doc->set_hermes("'123456.789'@CoolDecimalType(1,2)");
    assert_equals(true, doc->value()->is_typed_value());
    assert_throws<ResultException>([&](){
        doc->value()->as_double();
    });
});

}}
