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


class RandomBufferPopulator: public io::IOVectorProducer {
    const size_t mean_value_size_;
    Key key_cnt_{1};
    Value vv_{};
    uint64_t total_{};
    uint64_t total_max_;
public:
    RandomBufferPopulator(size_t mean_value_size, uint64_t total_max):
        mean_value_size_(mean_value_size), total_max_(total_max)
    {}

    virtual bool populate(io::IOVector& buffer)
    {
        for (int r = 0; r < 100; r++)
        {
            int32_t len = mean_value_size_; //getRandomG(mean_value_size_ * 2) + 1;
            buffer.symbol_sequence().append(0, 1);
            buffer.substream(0)->append(key_cnt_);
            total_ += sizeof(Key);

            buffer.symbol_sequence().append(1, len);
            buffer.substream(1)->reserve(sizeof(Value), len);

            total_ += len * sizeof(Value);

            key_cnt_++;
        }

        return total_ >= total_max_;
    }
};

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
        
        int64_t t0 = getTimeInMillis();
        
        RandomBufferPopulator provider(512 / sizeof(Value), 1024*1024*1024);

        map.begin().insert_subseq(provider, 512 * 1024 * 1024);

        //map.begin().dump();
        
        auto t1 = getTimeInMillis();
        
        std::cout << "Creation time: " << (t1 - t0) <<", size = " << map.size() << std::endl;

        // Finish snapshot so no other updates are possible.
        snp.commit();

        alloc.store("multimap_stream_data_iovec.dump");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
