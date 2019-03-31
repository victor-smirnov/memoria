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
#include <memoria/v1/api/vector/vector_api.hpp>

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

using Value = uint8_t;


class RandomBufferPopulator: public io::IOVectorProducer {    
    Value vv_{};
    uint64_t total_{};
    uint64_t total_max_;
public:
    RandomBufferPopulator(uint64_t total_max):
        total_max_(total_max)
    {}

    virtual bool populate(io::IOVector& buffer)
    {
        auto& seq = buffer.symbol_sequence();
        auto& s0 = io::substream_cast<io::IOColumnwiseFixedSizeArraySubstream>(buffer.substream(0));

        int32_t len = 128 * 1024;

        seq.append(0, len);
        s0.reserve(0, len);

        total_ += len * sizeof(Value);

        return total_ >= total_max_;
    }
};

int main()
{
    try {
        auto alloc = ThreadInMemAllocator<>::create();

        auto snp = alloc.master().branch();

        // Create Map
        auto ctr = create<Vector<Value>>(snp);

        ctr.new_block_size(128 * 1024);
        
        int64_t t0 = getTimeInMillis();
        
        RandomBufferPopulator provider(1024*1024*1024);

        ctr.begin().insert(provider);
        
        auto t1 = getTimeInMillis();
        
        std::cout << "Creation time: " << (t1 - t0) <<", size = " << ctr.size() << std::endl;

        // Finish snapshot so no other updates are possible.
        snp.commit();

        alloc.store("vector_stream_data_iovec.dump");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
