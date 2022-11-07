
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
#include <memoria/core/hermes/path/path.h>

#include <memoria/memoria.hpp>

#include <arrow/util/basic_decimal.h>
#include <arrow/util/decimal.h>

#include <unordered_map>

using namespace memoria;
using namespace memoria;
using namespace memoria::hermes;

template <typename T>
class Boo{};

class TTT {};

int main(int, char**)
{
    InitTypeReflections();

    auto doc = HermesCtr::parse_document(R"(
        [1,5,7,3,1,8,2,9,8,3]
    )");

    println("result: {}", doc->to_string());

    auto v1 = doc->root()->search("to_boolean(sort(@))");
    println("result: {}", v1->to_pretty_string());
    println("detached: {}", v1->is_detached());

    auto v2 = HermesCtr::wrap_dataobject<Double>(1234.5678);
    println("double: {}", v2->to_pretty_string());
    println("detached: {}", v2->is_detached());


}
