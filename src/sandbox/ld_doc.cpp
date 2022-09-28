
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

    pool::SharedPtr<HermesDoc> doc = HermesDoc::make_new();

    auto map = doc->set_generic_map();

    map->put<BigInt>("key1", -123456);
    map->put<Double>("key2", 5678);
    map->put<Varchar>("key3", "Hello world!");

    auto arr1 = map->put_generic_array("array1");
    arr1->append<Varchar>("abcde");
    arr1->append<Varchar>("efg");
    arr1->append_generic_map()->put<Varchar>("hijk", "lmnop");

    println("map.size: {}", map->size());
    println("map[{}]: {}", "key1", map->get("key1")->to_string());

    map->for_each([](auto k, auto v){
        println("Entry: {} :: {}", k, v->to_string());
    });

    println("{}", doc->to_pretty_string());
}
