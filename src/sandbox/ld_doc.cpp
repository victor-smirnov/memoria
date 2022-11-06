
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

    //    auto doc0 = "01000ull"_hdoc;
    //    println("{}", doc0->to_pretty_string());

    auto doc = HermesCtr::parse_document(R"(        
        class MyType<Parameter1<12345ull, true, -5679>>(1,2,3,4,5)
    )");

    println("{}", doc->to_string(StringifyCfg::simple()));
    println("{}", doc->root()->as_datatype()->to_cxx_string());

    auto hash = doc->root()->as_datatype()->cxx_type_hash();

    println("{}", hash);


}
