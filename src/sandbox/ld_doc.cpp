
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

//    auto tpl = parse_template(R"(
//        Prefix
//        {% for item in array1.array2 %}
//            {{ item }}
//        {% endfor %}
//        Suffix
//    )");

//    println("{}", tpl->to_string(StringifyCfg::pretty().with_raw_strings(false)));

//    auto data = HermesCtr::parse_document(R"({
//        array1: {
//            array2: <Double> [1, 2, 3.54321, 4, 5]
//        }
//    })");

//    render(tpl->root(), data->root(), std::cout);

    auto doc = HermesCtr::parse_document(R"(
        <Double> [
            1,
            -1,
            2_u32,
            1.23456789,
           5644449
        ]

    )");

    println("{}", doc->to_string(StringifyCfg::pretty().with_raw_strings(false)));

    return 0;
}
