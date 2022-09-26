
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

//using MapT = arena::Map<uint32_t, uint64_t>;
//using StdMap = std::unordered_map<uint32_t, uint64_t>;

//void check_maps(const StdMap& std_map, const MapT* map)
//{
//    if (map->size() != std_map.size()) {
//        println("Size mismatch: {} {}", std_map.size(), map->size());
//    }
//    else {
//        size_t cnt{};
//        map->for_each([&](auto k, auto v){
//            auto ii = std_map.find(k);
//            if (ii == std_map.end()) {
//                println("Key is not found: {}", k);
//            }
//            else if (ii->second != v) {
//                println("K/V mismatch: {} {} {}", k, v, ii->second);
//            }

//            cnt++;
//        });

//        if (cnt != map->size()) {
//            println("Map cnt size mismatch: cnt:{} size:{}", cnt, map->size());
//        }
//    }
//}

int main(int, char**)
{
    InitMemoriaExplicit();

    pool::SharedPtr<HermesDoc> doc = HermesDoc::make_new();

    auto map = doc->set_generic_map();

    map->put_varchar("key1", "value1");

    println("map.size: {}", map->size());
    println("map[{}]: {}", "key1", map->get("key1")->template cast_to<Varchar>()->view());

    map->for_each([](auto k, auto v){
        println("Entry: {} :: {}", k, v->template cast_to<Varchar>()->view());
    });

    println("{}", doc->to_pretty_string());

//    auto arr = doc->set_generic_array();

//    arr->append_string("Coll String");

//    println("{}", arr->size());
//    println("{}",arr->get(0)->template cast_to<String>()->view());

//    auto val = doc->set_string("Cool String");
//    println("Value: {}", val->view());

//    auto vv = doc->value();
//    println("{}", vv->is_string());

//    auto sv = cast_to<String>(vv);
//    println("{}", sv->view());

//    ArenaAllocator arena;

//    MapT* map = arena.allocate_object<MapT>();

//    StdMap std_map;

//    size_t size = 2000;

//    for (size_t c = 0; c < size; c++) {
//        println("insert: {}", c);
//        uint32_t key = getRandomG();
//        map->put(arena, key, c);
//        std_map[key] = c;
//        check_maps(std_map, map);
//    }

//    map->dump_state();

//    size_t cnt{};
//    while (std_map.size() && cnt < size)
//    {
//        auto key = std_map.begin()->first;
//        println("Erasing: {} :: {}", key, cnt);
//        map->remove(arena, key);
//        std_map.erase(key);
//        check_maps(std_map, map);
//        cnt++;
//    }

//    map->dump_state();

//    Boo b1;
//    //Boo b2 = b1;

//    println("Ptr: {} :: {} :: {} :: {}",
//            ptr.offset(),
//            (void*)ptr.get(),
//            std::is_trivially_copyable_v<RelativePtr<MemorySegment>>,
//            std::is_trivially_copyable_v<Boo>
//    );

}
