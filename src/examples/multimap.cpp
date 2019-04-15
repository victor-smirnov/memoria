// Copyright 2016 Victor Smirnov
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


// Idiomatic Map<K, V> example for Memoria. Map is an ordered mapping between keys and values
// backed with b-tree like structure. Besides traditional key-based lookup, Map<> also supports
// entry lookup by entry index from the beginning and bulk insertion/removal.

// Not every object type is supported as a key or value. In short, custom value codec or field factory
// must be defined. Out of the box, all primitive types are supported, String, BigInteger and some
// others (not yet implemented ones). Fixed-size types are somewhat faster on updates than
// variable-length (String, BigInteger) ones.

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/allocator/allocator_inmem_threads_api.hpp>
#include <memoria/v1/api/multimap/multimap_api.hpp>


#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/ticker.hpp>

#include <string>
#include <algorithm>
#include <map>

using namespace memoria::v1;
using namespace std;

template <typename V>
std::vector<V> make_vector(const V& value, size_t size) 
{
    std::vector<V> vv;
    for (int c = 0; c < size; c++) 
    {
        vv.emplace_back(value);
    }
    
    return vv;
}


std::vector<memoria::v1::UUID> read_uuids(const std::string& name)
{
    std::fstream file;
    file.open(name, std::ios_base::in);
    
    std::vector<memoria::v1::UUID> vv;
    
    while (file) 
    {
        std::string line;
        std::getline(file, line);
        if (file) { 
            vv.push_back(memoria::v1::UUID::parse(line.c_str()));
        }
    }
    
    return vv;
}

template <typename Key, typename Value, typename Iterator, typename EndIterator>
void checkCtr(CtrApi<Multimap<Key, Value>>& ctr, Iterator iter, EndIterator end) 
{
    auto t1 = getTimeInMillis();
    
    auto ii = ctr.begin();
    
    int32_t cnt{};
    while (iter != end)
    {
        auto id = ii.key();
        if (id != iter->first) 
        {
            std::cout << cnt << ": ID: " << id << " -- " << iter->first << (id != iter->first ? " -- !!!!!" : "") << std::endl;
        }
        
        if (ii.to_values()) 
        {
            auto vv = ii.read_values();
            
            if (vv.size() != iter->second.size()) 
            {
                std::cout << cnt << ": VAUES: " << id << " -- " << vv.size() << " : " << iter->second.size() << std::endl;
            }
        }
        else if (iter->second.size()) 
        {
            std::cout << cnt << ": VAUES: " << id << " -- " << 0 << " : " << iter->second.size() << std::endl;
        }
        
        cnt++;
        iter++;
    }
    
    auto t2 = getTimeInMillis();
    
    std::cout << "Check time: " << (t2 - t1) << std::endl;
}

int main()
{
    
    using Key   = int64_t;
    using Value = uint8_t;

    try {
        // Create persistent in-memory allocator for containers to store their data in.
        auto alloc = ThreadInMemAllocator<>::create();

        // This callocator is persistent not because it can dump its content to stream or file, but in a
        // narrow sense: to make a group of changes to allocator one must first start new snapshot by
        // branching form any exiting one's.

        // Snapshots are completely isolated from each other, operations don't interfere with operations
        // on other snapshots.

        auto snp = alloc.master().branch();


        // Create Map
        auto map = create<Multimap<Key, Value>>(snp);

        std::map<Key, std::vector<Value>> mapping;
        
        int32_t size = 10000;
        
        Ticker ticker(10000);
        
        int64_t t0 = getTimeInMillis();
        
        for (int c = 0; c < size; c++) 
        {
            auto key = getRandomG(10000); //UUID::make_random(); //uuids[c];
            auto val = make_vector<Value>(c, getRandomG(50000));
#ifdef MMA1_USE_IOBUFFER
            map.assign(key, val.begin(), val.end());
#endif

            mapping[key] = val;
            
            if (ticker.is_threshold()) 
            {
                std::cout << "Written: " << c << " in " << ticker.duration() << " ms" << std::endl;
                ticker.next();
            }
            
            ticker.tick();
        }
        
        auto t1 = getTimeInMillis();
        
        std::cout << "Creation time: " << (t1 - t0) <<", size = " << map.size() << std::endl;
        
        checkCtr(map, mapping.begin(), mapping.end());
        
        std::vector<Key> keys;
        
        for (auto& ee: mapping) {
            keys.push_back(ee.first);
        }
        
        std::random_shuffle(keys.begin(), keys.end());
        
        int32_t ccnt{};
        for (auto& key: keys) 
        {
            map.remove(key);
            mapping.erase(key);
            
            if (ccnt % 1000 == 0) 
            {
                checkCtr(map, mapping.begin(), mapping.end());
            }
            
            ccnt++;
        }
        
        checkCtr(map, mapping.begin(), mapping.end());
        
        
        // Dump readable contents of allocator to disk to see what is under the hood.
        // Mostly usable for hacking and debugging.
        //snp.dump("multimap.dir");

        // Finish snapshot so no other updates are possible.
        snp.commit();

        alloc.store("multimap_data.dump");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
