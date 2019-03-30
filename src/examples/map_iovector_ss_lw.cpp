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

    U8String key_;
    std::vector<int32_t> keys_lengths_;

    U8String value_;
    std::vector<int32_t> values_lengths_;

    uint8_t* key_buf_;
    uint8_t* value_buf_;

    int32_t key_len_;
    int32_t value_len_;

    size_t data_size_{};
    size_t total_data_size_;

public:
    RandomBufferPopulator(
            U8String key,
            U8String value,
            size_t block_size,
            size_t total_data_size
    ):
        key_(key), value_(value), total_data_size_(total_data_size)
    {
        ValueCodec<U8String> codec;

        key_len_ = codec.length(key_);
        value_len_ = codec.length(value_);

        key_buf_ = allocate_system<uint8_t>(key_len_).release();
        value_buf_ = allocate_system<uint8_t>(value_len_).release();

        codec.encode(key_buf_, key_, 0);
        codec.encode(value_buf_, value_, 0);

        size_t size0{};

        while (size0 < block_size)
        {
            keys_lengths_.push_back(key_len_);
            values_lengths_.push_back(value_len_);

            size0 += key_len_ + value_len_;
        }
    }

    virtual bool populate(io::IOVector& buffer)
    {
        auto& seq = buffer.symbol_sequence();
        auto& s0 = io::substream_cast<io::IOColumnwiseArraySubstream>(buffer.substream(0));
        auto& s1 = io::substream_cast<io::IOColumnwiseArraySubstream>(buffer.substream(1));

        auto key_buf   = s0.reserve(0, key_len_ * keys_lengths_.size(), keys_lengths_.size(), keys_lengths_.data());
        auto value_buf = s1.reserve(0, value_len_ * values_lengths_.size(), values_lengths_.size(), values_lengths_.data());

        seq.append(0, keys_lengths_.size());

        size_t key_pos{};
        size_t value_pos{};


        for (size_t c = 0; c < keys_lengths_.size(); c++, key_pos += key_len_, value_pos += value_len_)
        {
            std::memcpy(key_buf + key_pos, key_buf_, key_len_);
            std::memcpy(value_buf + value_pos, value_buf_, value_len_);

            data_size_ += key_len_ + value_len_;
        }

        return data_size_ >= total_data_size_;
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
        

        size_t total_data_size = 1024 * 1024 * 1024;

        RandomBufferPopulator provider("loooooooooooooooooooooooooooooooooooooooooooooo000ong key: 123", "loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo0ooong value: 123", 128 * 1024, total_data_size);



        int64_t t0 = getTimeInMillis();

        ctr.begin().insert(provider);
        
        auto t1 = getTimeInMillis();
        
        std::cout << "Creation time: " << (t1 - t0) <<", size = " << ctr.size() << std::endl;

        // Finish snapshot so no other updates are possible.
        snp.commit();

        alloc.store("map_ss_iovec_lw.dump");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
