
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
//    println("{:x}", ShortTypeCode::of_object(0x100ull).u64());

    InitTypeReflections();

    auto doc = HermesCtr::parse_document(R"(
{
  "people": [
    {
      "name": "a",
      "state": {"name": "up"}
    },
    {
      "name": "b",
      "state": {"name": "down"}
    },
    {
      "name": "c",
      "state": {"name": "up"}
    }
  ]
}
    )");

    println("result: {}", doc->to_string());

    auto v1 = doc->root()->search("people[].{Name: name, State: state.name}");
    println("result: {}", v1->to_pretty_string());
    println("detached: {}", v1->is_detached());

    auto v2 = HermesCtr::wrap_dataobject<Double>(1234.5678);
    println("double: {}", v2->to_pretty_string());
    println("detached: {}", v2->is_detached());

    auto array = doc->new_typed_array<Double>();
    array->append(12345.6789);

    println("{}", array->as_object()->to_pretty_string());
}
