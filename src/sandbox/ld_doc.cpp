
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

template <typename T>
class Boo{};

class TTT {};

int main(int, char**)
{
    InitTypeReflections();
    auto doc = HermesDoc::parse(R"(
        [0b10011101ull]
    )");

    ValuePtr dvv = doc->value();

    GenericArrayPtr vv = dvv->template cast_to<GenericArray>();

    vv->append_hermes("{field1: CollType<ABC>('A', 1, 2, 3)}");


    println("{}", doc->to_string());

    println("{}", TypeNameFactory<Boo<unsigned short int>>::name());
    println("{}", TypeNameFactory<Boo<TTT>>::name());
    println("{}", std::is_same_v<int const volatile*, const volatile int*>);


/*

        #{ref0001: int}
        [[
            unsigned long int, unsigned long, unsigned long long,
            unsigned int, unsigned short, unsigned char, unsigned,
            unsigned short int,
            '--------------',
            long double,
            double, float, bool,
            '--------------',
            signed, signed int, signed long,
            signed long long,
            signed long int,
            signed short,
            '--------------',
            int, short, short int, char, long, long long, long int**&
        ],
        boo<T&, int, 1, 2, true, false> const volatile* volatile&&,
        'привет мир'
        ]


*/
}
