
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

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
