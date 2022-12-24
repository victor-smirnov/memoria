
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
    auto doc = hermes::HermesCtr::make_new();

    doc->set_dataobject<Double>(12345.67);
    assert_equals(true, doc->root().is_double());
    assert_equals(12345.67, doc->root().as_double());
    assert_throws<ResultException>([&](){
        doc->root().as_bigint();
    });


    doc->set_dataobject<Varchar>("Hello world");
    assert_equals(true, doc->root().is_varchar());
    assert_equals("Hello world", doc->root().as_varchar());

    assert_throws<ResultException>([&](){
        doc->root().as_double();
    });

    doc->set_dataobject<BigInt>(123);
    assert_equals(true, doc->root().is_bigint());
    assert_equals(123, doc->root().as_bigint());

    assert_throws<ResultException>([&](){
        doc->root().as_double();
    });

    doc->set_dataobject<Boolean>(1);
    assert_equals(true, doc->root().is_boolean());
    assert_equals(1, doc->root().as_boolean());

    assert_throws<ResultException>([&](){
        doc->root().as_double();
    });

    doc->set_hermes("12345ll");
    assert_equals(true, doc->root().is_bigint());
    assert_equals(12345, doc->root().as_bigint());

    doc->set_hermes("{}");
    assert_equals(true, doc->root().is_map());
    assert_equals(0, doc->root().as_generic_map()->size());
    assert_throws<ResultException>([&](){
        doc->root().as_double();
    });

    doc->set_hermes("[1,2,3,4]");
    assert_equals(true, doc->root().is_array());
    assert_equals(4, doc->root().as_generic_array()->size());
    assert_throws<ResultException>([&](){
        doc->root().as_double();
    });

    doc->set_hermes("Decimal(1,2)");
    assert_equals(true, doc->root().is_datatype());
    assert_throws<ResultException>([&](){
        doc->root().as_double();
    });

    doc->set_null();
    assert_equals(true, doc->root().is_null());
    assert_throws<ResultException>([&](){
        doc->root().as_double();
    });

    doc->set_hermes("'123456.789'@CoolDecimalType(1,2)");
    assert_equals(true, doc->root().is_typed_value());
    assert_throws<ResultException>([&](){
        doc->root().as_double();
    });
});

}}
