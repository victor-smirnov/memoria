
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
#include <memoria/core/tools/time.hpp>

#include <arrow/util/basic_decimal.h>
#include <arrow/util/decimal.h>

#include <boost/pool/pool_alloc.hpp>

#include <unordered_map>

using namespace memoria;
using namespace memoria;
using namespace memoria::hermes;

int main(int, char**)
{
    InitTypeReflections();

    //reservations[].instances[].[tags[?Key=='Name'].Values[] | [0], type, state.name]

//    auto expr = R"(
//        myarray[?contains(@, 'foo') == ^true]
//    )";

//    auto doc = HermesCtr::parse_hermes_path(expr);
//    println("result: {}", doc->to_pretty_string());

    auto doc = HermesCtr::parse_document(R"(
{
  "locations": [
    {"name": "Seattle", "state": "WA"},
    {"name": "New York", "state": "NY"},
    {"name": "Bellevue", "state": "WA"},
    {"name": "Olympia", "state": "WA"}
  ]
}
    )");

    auto doc2 = doc->root()->search(R"(locations[?state == 'WA'].name | sort(@)[-2:] | {WashingtonCities: join(', ', @)})");
    println("result: {}", doc2->to_pretty_string());

    return 0;
}
