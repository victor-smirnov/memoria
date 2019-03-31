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
#include <memoria/v1/api/map/map_api.hpp>

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

using Key   = U8String;
using Value = U8String;


class RandomBufferPopulator: public io::IOVectorProducer {    
    size_t idx_{};

    std::vector<U8String>& keys_;
    std::vector<int32_t>& keys_lengths_;

    std::vector<U8String>& values_;
    std::vector<int32_t>& values_lengths_;

public:
    RandomBufferPopulator(
            std::vector<U8String>& keys, std::vector<int32_t>& keys_lengths,
            std::vector<U8String>& values, std::vector<int32_t>& values_lengths
    ):
        keys_(keys), keys_lengths_(keys_lengths), values_(values), values_lengths_(values_lengths)
    {}

    virtual bool populate(io::IOVector& buffer)
    {
        auto& seq = buffer.symbol_sequence();
        auto& s0 = io::substream_cast<io::IOColumnwiseVLenArraySubstream>(buffer.substream(0));
        auto& s1 = io::substream_cast<io::IOColumnwiseVLenArraySubstream>(buffer.substream(1));

        int32_t max_batch_size = 128 * 1024;
        int32_t batch_size{};

        ValueCodec<U8String> codec;

        while (idx_ < keys_.size() && batch_size < max_batch_size)
        {
            size_t diff = keys_.size() - idx_;
            size_t block_size = (diff >= 100) ? 100 : diff;

            seq.append(0, block_size);

            int32_t keys_data_size{};
            int32_t values_data_size{};

            for (size_t c = idx_; c < idx_ + block_size; c++)
            {
                keys_data_size += keys_lengths_[c];
                values_data_size += values_lengths_[c];
            }

            auto key_buf   = s0.reserve(0, block_size, &keys_lengths_[idx_]);
            auto value_buf = s1.reserve(0, block_size, &values_lengths_[idx_]);

            size_t key_pos{};
            size_t value_pos{};

            for (size_t c = idx_; c < idx_ + block_size; c++)
            {
                key_pos += codec.encode(key_buf, keys_[c], key_pos);
                value_pos += codec.encode(value_buf, values_[c], value_pos);
            }


            batch_size += keys_data_size + values_data_size;
            idx_ += block_size;
        }

        //std::cout << "Blk: " << idx_ << " " << batch_size << std::endl;

        return idx_ >= keys_.size();
    }
};





int main()
{
    try {
        auto alloc = ThreadInMemAllocator<>::create();

        auto snp = alloc.master().branch();

        // Create Map
        auto ctr = create<Map<Key, Value>>(snp);

        ctr.new_block_size(32 * 1024);
        

        std::vector<U8String> keys;
        std::vector<U8String> values;
        std::vector<int32_t> keys_lengths;
        std::vector<int32_t> values_lengths;

        size_t total_data_size = 1024 * 1024 * 1024;
        size_t size{};

        ValueCodec<U8String> codec;

        size_t idx{};
        while (size < total_data_size)
        {
            U8String tmp = std::to_string(idx++);
            U8String key   = U8String("loooooooooooooooooooooooooooooooooooooooooooooo000ong key: ") + tmp;
            U8String value = U8String("loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo0ooong value: ") + tmp;

            keys_lengths.push_back(codec.length(key));
            keys.push_back(std::move(key));

            values_lengths.push_back(codec.length(value));
            values.push_back(std::move(value));

            size += keys_lengths[keys_lengths.size() - 1] + values_lengths[values_lengths.size() - 1];
        }

        RandomBufferPopulator provider(keys, keys_lengths, values, values_lengths);

        std::cout << "Inserting " << keys.size() << " k/v pairs" << std::endl;

        int64_t t0 = getTimeInMillis();

        ctr.begin().insert(provider);
        
        auto t1 = getTimeInMillis();
        
        std::cout << "Creation time: " << (t1 - t0) <<", size = " << ctr.size() << std::endl;

        // Finish snapshot so no other updates are possible.
        snp.commit();

        alloc.store("map_ss_iovec.dump");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
