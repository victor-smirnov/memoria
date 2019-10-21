
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/mapped/mapped.hpp>

#include <memoria/v1/core/strings/u8_string.hpp>

#include <iostream>
#include <string>
#include <unordered_map>

using namespace memoria::v1;

using ArenaT = MappedArena<int64_t>;

template <typename T>
using PtrT = typename ArenaT::template PtrT<T>;

template <typename T>
size_t count(const T& map) {
    size_t cnt = 0;

    map.for_each([&](const auto& kk, const auto& vv){
        cnt++;
    });

    return cnt;
}

int main()
{
    std::map<U8String, U8String> std_map;

    for (size_t c = 0; c < 100000; c++)
    {
        auto key = std::string("Booo_") + std::to_string(c);
        auto value = std::string("Fooo_") + std::to_string(c);
        std_map[key] = value;
    }

    ArenaT arena;

    using Key   = MappedString<>;
    using Value = MappedString<>;

    using KeyPtr = PtrT<Key>;
    using ValuePtr = PtrT<Value>;

    using Map = MappedMap<KeyPtr, ValuePtr, typename ArenaT::ArenaBase, MappedPtrHashFn, MappedStringPtrEqualToFn>;
    Map map = Map::create(&arena);

    for (auto& entry: std_map)
    {
        KeyPtr key = allocate<Key>(&arena, entry.first);
        ValuePtr value = allocate<Key>(&arena, entry.second);

        map.put(key, value);
    }

    std::cout << "Size: " << map.size() << std::endl;

    map.for_each([&](auto kk, auto vv){
        U8String key = kk.get(&arena)->view();
        auto ii = std_map.find(key);
        if (ii == std_map.end()) {
            std::cout << "Not found! " << key << std::endl;
        }
    });

    for (auto& entry: std_map)
    {
        auto val = map.get(entry.first);

        if (val) {
            auto vv_view = val.get().get(&arena)->view();

            if (vv_view != entry.second.to_std_string()) {
                std::cout << "Value mismatch! " << vv_view << " :: " << entry.second << std::endl;
            }
        }
        else {
            std::cout << "Not found! " << entry.first << std::endl;
        }
    }

    for (auto& entry: std_map)
    {
        auto val = map.remove(entry.first);

        if (!val) {
            std::cout << "Not found! " << entry.first << std::endl;
        }

        size_t cnt = count(map);
        if (cnt != map.size()) {
            std::cout << "Size mismatch! " << cnt << " :: " << map.size() << std::endl;
        }

        std::cout << map.size() << " :: " << arena.size() << std::endl;
    }


    std::cout << "Size: " << map.size() << std::endl;

    return 0;
}
