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

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/ticker.hpp>

#include <string>
#include <algorithm>
#include <map>
#include <vector>

using namespace memoria::v1;
using namespace std;

using Key   = int64_t;
using Value = uint8_t;

void check_values(absl::Span<const Value> span)
{
    Value cc = 0; size_t cnt{};
    for (auto vv: span)
    {
        if (MMA1_UNLIKELY(cc != vv))
        {
            std::cout << cnt << ") PFX NOT EQUALS" << std::endl;
        }

        cc++;
        cnt++;
    }
}

bool check_values_b(absl::Span<const Value> span)
{
    Value cc = 0; size_t cnt{};
    for (auto vv: span)
    {
        if (MMA1_UNLIKELY(cc != vv))
        {
            std::cout << cnt << ") PFX NOT EQUALS" << std::endl;
            return false;
        }

        cc++;
        cnt++;
    }

    return true;
}


int main()
{
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
        //map.new_block_size(128 * 1024);
        
        std::vector<Value> data1(4090*3);
        map.upsert(1, data1);

        //map.keys()->dump_iterator();

        std::vector<Value> data2(409);
        map.append_entry(2, data2);

        //map.keys()->dump_iterator();

        map.remove_all(1, 2);

        // Finish snapshot so no other updates are possible.
        snp.commit();

        alloc.store("multimap_stream_data_iovec.mma1");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
