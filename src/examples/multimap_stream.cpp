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

#include <memoria/v1/containers/multimap/mmap_input.hpp>

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
using Value = uint64_t;

mmap::MapData<Key, Value> generate_data(size_t entries, size_t mean_entry_size)
{
    mmap::MapData<Key, Value> data;

    for (int c = 0; c < entries; c++)
    {
        std::vector<Value> vv(getRandomG(mean_entry_size * 2));
        data.push_back(std::make_pair(c, std::move(vv)));
    }

    return data;
}

template <typename IOBufferT>
class RandomBufferPopulator: public bt::BufferProducer<IOBufferT> {
    const size_t mean_value_size_;
    Key key_cnt_{1};
    Value vv_{};
    uint64_t total_{};
    uint64_t total_max_;
public:
    RandomBufferPopulator(size_t mean_value_size, uint64_t total_max):
        mean_value_size_(mean_value_size), total_max_(total_max)
    {}

    virtual int32_t populate(IOBufferT& buffer)
    {
        int32_t entries{};
        while (true)
        {
            size_t pos = buffer.pos();
            if (buffer.template putSymbolsRun<2>(0, 1) && buffer.put(key_cnt_))
            {
                key_cnt_++;
                entries += 2;
                total_ += sizeof(Key);
            }
            else {
                buffer.pos(pos);
                break;
            }

            int32_t v_size = mean_value_size_;//getRandomG(mean_value_size_ * 2) + 1;
            int32_t v_data_size = v_size * sizeof(Value);
            int32_t v_full_data_size = v_data_size + 8;

            if (buffer.capacity() >= v_full_data_size)
            {
                buffer.template putSymbolsRun<2>(1, v_size);
                for (int c = 0; c < v_size; c++) {
                    buffer.put(vv_);
                }

                entries += v_size + 1;
                total_ += sizeof(Value) * v_size;
            }
            else if (buffer.capacity() > 8 + sizeof(Value))
            {
                int32_t av_size = (buffer.capacity() - 8) / sizeof(Value);
                if (av_size > 0)
                {
                    if (buffer.template putSymbolsRun<2>(1, av_size))
                    {
                        for (int c = 0; c < av_size; c++) {
                            if (!buffer.put(vv_)) {
                                MMA1_THROW(RuntimeException()) << WhatCInfo("Ivalid buffer (2)");
                            }
                        }

                        entries += av_size + 1;
                        total_ += sizeof(Value) * av_size;
                    }
                    else {
                        MMA1_THROW(RuntimeException()) << WhatCInfo("Ivalid buffer (1)");
                    }
                }

                break;
            }
            else {
                break;
            }
        }

        return entries * (total_ < total_max_ ? 1 : -1);
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
        
        auto data = generate_data(1000000, 128);


        int64_t t0 = getTimeInMillis();
        
        mmap::MultimapIOBufferProducer<Key, Value> provider(data);

        //RandomBufferPopulator<DefaultIOBuffer> provider(128, 1000000000);

        map.begin().insert_subseq(provider);
        
        auto t1 = getTimeInMillis();
        
        std::cout << "Creation time: " << (t1 - t0) <<", size = " << map.size() << std::endl;

        // Finish snapshot so no other updates are possible.
        snp.commit();

        alloc.store("multimap_stream_data.dump");
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
