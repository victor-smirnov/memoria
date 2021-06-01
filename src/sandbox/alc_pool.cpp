
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

#include <iostream>

using namespace memoria;


namespace memoria {
    void InitCoreLDDatatypes();
}

int main(void) {
    InitCoreLDDatatypes();

    try {
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
