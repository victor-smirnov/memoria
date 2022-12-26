
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


#include <memoria/core/strings/format.hpp>

#include <memoria/core/tools/span.hpp>

#include <memoria/core/tools/random.hpp>

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/hermes/path/path.h>

#include <memoria/memoria.hpp>
#include <memoria/core/tools/time.hpp>

#include <memoria/core/numbers/number_view.hpp>


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
//        {%+ for item in array1.array2 -%}
//            {{ item }}
//        {%- endfor +%}
//        Suffix
//    )");

//    auto tpl = parse_template(R"(
//        Prefix
//        {% if array1.array2 %}
//            {{ array1.array2[0] }}
//        {% elif ^true %}
//        {% else %}
//        {% endif %}
//        Suffix
//    )");

    auto tpl = parse_template(R"(
        Prefix
        {%- if array1.array2 %}
            {{ array1.array2[0] }}
        {% elif ^true -%}
            |Some Text|
        {%- else %}
        {% endif -%}
        Suffix
)");

    println("{}", tpl->to_string(StringifyCfg::pretty().with_raw_strings(false)));

    auto data = HermesCtr::parse_document(R"({
        array1: {
            array2: <memoria::Double> [1] //1, 2, 3.54321, 4, 5
        }
    })");

    render(tpl->root(), data->root(), std::cout);

    using DblView    = NumberView<double, ViewKind::BY_VALUE>;
    using DblViewPtr = Own<DblView>;

    DblViewPtr v1(0.123456);
    DblViewPtr v2(1.0);
    DblViewPtr v3;
    DblViewPtr v4;

    v3 = v2;

    v4 = 777;

    auto vsum = v1 + v2 + 8;

    println("{}", v1);
    println("{}", v2);
    println("{}", v3);
    println("{}", v4);
    println("{} :: {}", vsum, TypeNameFactory<decltype(vsum)>::name());


    std::vector<double> vals = {1,2,3,4,5};


    auto docx = HermesCtr::make_new();
    std::vector<Object> objs = {
        docx->make(1),
        docx->make(2.3456),
        docx->make("hello world"),
        docx->make(true),
    };

    //auto val = doc->make<double>(12345);

    auto val = docx->make_array_t<BigInt>(vals);
    println("{}", val.as_object().to_pretty_string());

    Object obj = val;

    return 0;
}
