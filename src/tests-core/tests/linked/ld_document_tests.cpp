
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

auto document_set_test = register_test_in_suite<FnTest<LDTestState>>("LDDocumentTestSuite", "Document", [](auto& state){
    LDDocument doc;

    doc.set_double(12345.67);
    assert_equals(true, doc.value().is_double());
    assert_equals(12345.67, doc.value().as_double());
    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_bigint();
    });


    doc.set_varchar("Hello world");
    assert_equals(true, doc.value().is_varchar());
    assert_equals("Hello world", doc.value().as_varchar().view());

    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_double();
    });

    doc.set_bigint(123);
    assert_equals(true, doc.value().is_bigint());
    assert_equals(123, doc.value().as_bigint());

    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_double();
    });

    doc.set_boolean(1);
    assert_equals(true, doc.value().is_boolean());
    assert_equals(1, doc.value().as_boolean());

    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_double();
    });

    doc.set_sdn("12345");
    assert_equals(true, doc.value().is_bigint());
    assert_equals(12345, doc.value().as_bigint());

    doc.set_sdn("{}");
    assert_equals(true, doc.value().is_map());
    assert_equals(0, doc.value().as_map().size());
    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_double();
    });

    doc.set_sdn("[]");
    assert_equals(true, doc.value().is_array());
    assert_equals(0, doc.value().as_array().size());
    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_double();
    });

    doc.set_sdn("Decimal(1,2)");
    assert_equals(true, doc.value().is_type_decl());
    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_double();
    });

    doc.set_null();
    assert_equals(true, doc.value().is_null());
    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_double();
    });

    doc.set_sdn("'123456.789'@CoolDecimalType(1,2)");
    assert_equals(true, doc.value().is_typed_value());
    assert_throws<LDDInvalidCastException>([&](){
        doc.value().as_double();
    });


    LDDocumentView immutable_doc_view = doc.as_immutable_view();

    assert_throws<RuntimeException>([&](){
        immutable_doc_view.make_mutable();
    });

    assert_throws<RuntimeException>([&](){
        immutable_doc_view.set_null();
    });


    LDDocumentView doc_view = doc;

    assert_no_throws<RuntimeException>([&](){
        doc_view.make_mutable();
    });

    assert_equals(true, doc_view.value().is_typed_value());

    LDDocument doc2;
    doc2.set_varchar("Hello World");

    doc.set_document(doc2);
    assert_equals(true, doc.value().is_varchar());
    assert_equals("Hello World", doc.value().as_varchar().view());


    LDTypeDeclarationView type = doc.create_named_type("Type1", "CoolType2");

    assert_equals("CoolType2", type.name());
    doc.set_sdn("'12345'@#Type1");
    assert_equals(true, doc.value().is_typed_value());
    LDDTypedValueView tval = doc.value().as_typed_value();
    assert_equals(type, tval.type());


    auto types = doc.named_types();
    assert_equals(1, types.size());
});

}}}
