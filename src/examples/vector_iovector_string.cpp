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
#include <memoria/v1/api/vector/vector_api.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/ticker.hpp>

#include <memoria/v1/core/strings/string_codec.hpp>

#include <string>
#include <algorithm>
#include <map>
#include <vector>

using namespace memoria::v1;
using namespace std;

using Value = U8String;


class RandomBufferPopulator: public io::IOVectorProducer {    
    Value vv_{};
    uint64_t total_{};
    uint64_t total_max_;

    std::vector<U8String> strings;
    std::vector<uint32_t> lengths;

    ValueCodec<Value> codec{};

public:
    RandomBufferPopulator(uint64_t total_max):
        total_max_(total_max)
    {
        for (int c = 0; c < 1024; c++)
        {
            U8String str = "loooooooooooooooong ooooooooooooooooooooooooooooooooooooooooo value_" + std::to_string(c);

            int32_t len = codec.length(str);

            lengths.push_back(len);
            strings.push_back(std::move(str));
        }
    }

    virtual bool populate(io::IOVector& buffer)
    {
        auto& seq = buffer.symbol_sequence();
        auto& s0 = io::checked_substream_cast<io::IORowwiseVLenArraySubstream<Value>>(buffer.substream(0));

        int32_t max_len = 128 * 1024;
        std::vector<int32_t> values;
        std::vector<int32_t> sizes;

        int32_t total_len{};

        while (total_len < max_len)
        {
            int32_t idx = getRandomG(strings.size());
            int32_t len = lengths[idx];

            values.push_back(idx);
            sizes.push_back(len);

            total_len += len;
        }

        seq.append(0, values.size());
        auto wrt_buffer = T2T<typename ValueCodec<Value>::BufferType*>(s0.reserve(sizes.size(), sizes.data()));

        size_t pos{};
        for (size_t c = 0; c < values.size(); c++)
        {
            pos += codec.encode(wrt_buffer, strings[values[c]], pos);
        }

        total_ += total_len;

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

        ctr.new_block_size(32 * 1024);
        
        int64_t t0 = getTimeInMillis();
        
        RandomBufferPopulator provider(1024*1024*1024);

        ctr.begin().insert(provider);
        
        auto t1 = getTimeInMillis();
        
        std::cout << "Creation time: " << (t1 - t0) <<", size = " << ctr.size() << std::endl;

        // Finish snapshot so no other updates are possible.
        snp.commit();

        //alloc.store("vector_stream_data_iovec_strng.dump");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
