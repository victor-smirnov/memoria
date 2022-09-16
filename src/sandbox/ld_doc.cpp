
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

#include <memoria/core/datatypes/arena/arena.hpp>
#include <memoria/core/datatypes/arena/vector.hpp>
#include <memoria/core/datatypes/arena/map.hpp>
#include <memoria/core/datatypes/arena/traits.hpp>

#include <unordered_map>

using namespace memoria;
using namespace memoria::arena;

using MapT = Map<uint32_t, uint64_t>;
using StdMap = std::unordered_map<uint32_t, uint64_t>;

void check_maps(const MemorySegment* sgm, const StdMap& std_map, const MapT* map)
{
    if (map->size() != std_map.size()) {
        println("Size mismatch: {} {}", std_map.size(), map->size());
    }
    else {
        size_t cnt{};
        map->for_each(sgm, [&](auto k, auto v){
            auto ii = std_map.find(k);
            if (ii == std_map.end()) {
                println("Key is not found: {}", k);
            }
            else if (ii->second != v) {
                println("K/V mismatch: {} {} {}", k, v, ii->second);
            }

            cnt++;
        });

        if (cnt != map->size()) {
            println("Map cnt size mismatch: cnt:{} size:{}", cnt, map->size());
        }
    }
}

int main(int, char**)
{
    ArenaSegmentImpl arena(128);

    auto map_ptr = arena.template allocate<MapT>();

    MapT* map = map_ptr.write(&arena);

    StdMap std_map;

    for (size_t c = 0; c < 2000; c++) {
        println("insert: {}", c);
        map = map->put(&arena, c, c);
        std_map[c] = c;
        check_maps(&arena, std_map, map);
    }

    map->dump_state(&arena);

    size_t cnt{};
    while (std_map.size() && cnt < 2000)
    {
        auto key = std_map.begin()->first;
        println("Erasing: {} :: {}", key, cnt);
        map = map->remove(&arena, key);
        std_map.erase(key);
        check_maps(&arena, std_map, map);
        cnt++;
    }

    map->dump_state(&arena);
}
