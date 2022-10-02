
// Copyright 2022 Victor Smirnov
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
#include <memoria/core/strings/format.hpp>

#include <memoria/core/tools/span.hpp>

#include <memoria/core/tools/random.hpp>

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/memoria.hpp>

#include <unordered_map>

using namespace memoria;
using namespace memoria;
using namespace memoria::hermes;



int main(int, char**)
{
    InitTypeReflections();

    auto doc = HermesDoc::parse(R"(
        {some_key: 1234}
    )");

    println("{}", doc->to_string());

//    auto doc = HermesDoc::parse(R"(
//        @abcd<K,L,M>(1,2,3, aaa) = {abcd: "Cool world"}
//    )");

//    println("{}", doc->to_string());

//    auto doc2 = HermesDoc::parse_datatype(R"(
//        abcd<K,L,M>(1,2,3, aaa)
//    )");

//    println("{}", doc2->to_string());


//    auto doc3 = LDDocument::parse(R"(some_id)");

//    println("{}", doc3->to_string());
}
