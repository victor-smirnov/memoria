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
        auto& seq = buffer.symbol_sequence();
        auto& s0 = io::substream_cast<io::IOColumnwiseFixedSizeArraySubstream<Key>>(buffer.substream(0));
        auto& s1 = io::substream_cast<io::IORowwiseFixedSizeArraySubstream<Value>>(buffer.substream(1));

        for (int r = 0; r < 1000; r++)
        {
            int32_t len = getRandomG(mean_value_size_ * 2) + 1; //mean_value_size_; //
            seq.append(0, 1);
            s0.append(0, key_cnt_);
            total_ += sizeof(Key);

            seq.append(1, len);
            Value* val = s1.reserve(len);

            for (int c = 0; c < len; c++)
            {
                *(val + c) = c;
            }

            total_ += len * sizeof(Value);

            key_cnt_++;
        }

        return total_ >= total_max_;
    }
};

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
        map.new_block_size(512 * 1024);
        
        int64_t t0 = getTimeInMillis();
        
        RandomBufferPopulator provider(512 / sizeof(Value), 1024 * 1024 * 1024);

        map.begin().insert(provider); //127 sq in

        auto t1 = getTimeInMillis();
        
        std::cout << "Creation time: " << (t1 - t0) << ", size = " << map.size() << std::endl;

        auto ii2 = map.seek_e(0);
        ii2->set_buffered();

        io::DefaultIOBuffer<Value> suffix_buffer(4096);

        auto t4= getTimeInMillis();
//        int cnt2 = 0;
//        while (!ii2->is_end())
//        {
//            if ((!ii2->is_first_iteration()) && ii2->is_buffer_ready())
//            {
//                check_values(ii2->buffer());
//            }

//            if (ii2->has_entries())
//            {
//                for (auto& entry: ii2->entries())
//                {
//                    check_values(entry.values);
//                }
//            }

//            cnt2++;
//            ii2->next();
//        }

//        check_values(ii2->buffer());

        auto t5 = getTimeInMillis();

//        std::cout << "Entries Iteration time: " << (t5 - t4) << ", cnt = " << cnt2 << std::endl;

        auto t6 = getTimeInMillis();

        int64_t map_size = map.size();
//        for (int64_t c = 0; c < map_size; c++)
//        {
//            //auto ii = map.seek(getRandomG(map_size));
//            //std::cout << c << std::endl;
//            auto ii = map.seek_e(getRandomG(map_size));
//            ii->set_buffered();

//            if (ii->has_entries())
//            {
//                auto& entry = ii->entry(0);
//                check_values(entry.values);
//            }
//            else {
//                do {
//                    ii->next();
//                }
//                while (!ii->is_buffer_ready());
//                check_values(ii->buffer());
//            }
//        }


        for (int64_t c = 0; c < map_size; c++)
        {
            auto key = getRandomG(map_size);
            auto ii = map.find_v(key);

//            if (ii)
//            {
//                ii->set_buffered();

//                while (!ii->is_buffer_ready()) {
//                    ii->next();
//                }

//                if (!check_values_b(ii->buffer()))
//                {
//                    auto jj = map.find_v(key);
//                    ii->set_buffered();

//                    while (!jj->is_buffer_ready()) {
//                        jj->dump_iterator();
//                        jj->next();
//                    }

//                    jj->dump_iterator();
//                    check_values_b(ii->buffer());
//                }
//            }
        }


        auto t7 = getTimeInMillis();

        std::cout << "Entries Iteration (2) time: " << (t7 - t6) << std::endl;

        // Finish snapshot so no other updates are possible.
        snp.commit();

        //alloc.store("multimap_stream_data_iovec.dump");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
