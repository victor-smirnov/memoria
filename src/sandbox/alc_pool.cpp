
// Copyright 2021 Victor Smirnov
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
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/result.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/regexp/icu_regexp.hpp>

#include <iostream>

using namespace memoria;


namespace memoria {
    void InitCoreLDDatatypes();
}


int main(void) {
    InitCoreLDDatatypes();

    println("{}", UUID{0, 594});

    try {

    LDDocument doc1 = LDDocument::parse(R"(
    @CodegenConfig = {
        "groups": {
            "default": {
                "datatypes": @FileGenerator = {
                    "filename": "src/containers/generated/ctr_datatypes.cpp",
                    "handler": "codegen.datatypes.DatatypesInitSink"
                },
                "containers": @TypeInstance = {
                    "path": "src/containers/generated"
                }
            },

            "stores": {
                "datatypes": @FileGenerator = {
                    "filename": "src/stores/generated/ctr_datatypes.cpp",
                    "handler": "codegen.datatypes.DatatypesInitSink"
                },
                "containers": @TypeInstance = {
                    "path": "src/stores/generated"
                }
            }
        },
        "profiles": ["CowProfile<>", "NoCowProfile<>", "CowLiteProfile<>"],
        "script": "codegen/python/codegen.py"
    }
)");

        println("{}", doc1.to_pretty_string());

        LDDocumentView dv = doc1;


        auto vv0 = dv.value();
        auto res = find_value(vv0, "$/groups/default/containers/$/path");

        println("{} :: {}", res, vv0.to_standard_string());

        UUID a1 = UUID::parse("d4cde295-5a91-422c-9be7-9caf6ff8e08a");
        UUID a2 = UUID::parse("7f9a674f-5e6c-4c8c-a60d-b084786453be");


        println("a1 < a2 :: {}", a1 < a2);


        LDDocument doc = LDDocument::parse_type_decl("Map<UUID, CommitMetadataDT<CoreApiProfile>>");

        auto td = doc.value().as_type_decl();

        println("params: {}", td.is_parametric());

        auto p2 = td.get_type_declration(1);

        println("params: {}", p2.is_parametric());

        println("{}", p2.to_standard_string());
    }
    catch (const MemoriaThrowable& ee) {
        ee.dump(std::cout);
    }
    catch (const std::exception& ee) {
        std::cerr << "Exception: " << ee.what() << std::endl;
    }
}
