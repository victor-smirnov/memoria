
// Copyright 2011 Victor Smirnov
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


#include <memoria/v1/core/tools/lfu_cache.hpp>
#include <random>

using namespace memoria;

typedef std::mt19937 MyRNG;

template <typename Node>
struct EvictionPredicate {
    bool operator()(const Node& node) {return node.value_ > 10 ;}
};

template <typename Node>
struct LRUCacheNode: Node {
    Int value_;
};


Int main() {

    MyRNG rng;

    std::uniform_int_distribution<int32_t> dist(0,100);

    typedef LFUCache<Int, EvictionPredicate, LRUCacheNode>  CacheType;
    typedef CacheType::Entry                                Entry;

    CacheType cache(40);

    for (Int c = 0; c < 1000000; c++)
    {
        int32_t key = dist(rng);
        cache.get_entry(key, [](Entry& entry) {
            entry.value_ = entry.key();
        });
    }

    cache.dump();
    cache.checkOrder();

    return 0;
}
