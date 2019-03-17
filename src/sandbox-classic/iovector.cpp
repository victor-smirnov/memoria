
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

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/allocator/allocator_inmem_threads_api.hpp>

#include <memoria/v1/api/multimap/multimap_api.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <iostream>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <random>
#include <thread>

using namespace memoria::v1;

using Key = int64_t;
using Value = uint64_t;


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

    virtual bool populate(io::IOVector& io_vectors)
    {
        io::IOSymbolSequence& seq = io_vectors.symbol_sequence();
        auto& keys   = io::substream_cast<io::IOArraySubstream>(io_vectors.substream(0));
        auto& values = io::substream_cast<io::IOArraySubstream>(io_vectors.substream(1));

        while (true)
        {
            seq.append(0, 1);
            keys.append(key_cnt_);

            key_cnt_++;
            total_ += sizeof(Key);

            int32_t v_size = getRandomG(mean_value_size_ * 2) + 1;
            seq.append(1, v_size);

            for (int c = 0; c < v_size; c++) {
                values.append(vv_);
            }

            total_ += v_size * sizeof(Value);
        }

        return total_ < total_max_;
    }
};

struct Boo {
    uint8_t mas[16];
};

io::IOArraySubstream* make_stream(int32_t capacity)
{
    return new io::IOArraySubstreamTypedFixedSizeGrowable<Boo>(capacity);
}

int main(int argc, char** argv, char** envp)
{
    try {
        io::IOArraySubstream* stream = make_stream(1024*1024*1024);

        int64_t t_start = getTimeInMillis();

        int32_t batch_size = 64;

        for (int c = 0; c < 1024*1024*1024/4/batch_size; c++)
        {
            uint8_t* ptr = stream->reserve(sizeof(int32_t) * batch_size, batch_size);
            for (int d = 0; d < batch_size * 4; d += 4) {
                *T2T<uint32_t*>(ptr + d) = c + d;
            }
        }

        int64_t t_end = getTimeInMillis();

        std::cout << fmt::format(u"Time: {} ms :: {}", t_end - t_start, 0) << std::endl;


//        auto alloc = ThreadInMemAllocator<>::create();

//        RandomBufferPopulator pop(128, 10000);

//        io::PackedSymbolSequenceOwningImpl<2> seq0;
//        for (int c = 0; c < 500; c++) {
//            seq0.append(c % 2, c + 1);
//        }

//        seq0.reindex();

//        seq0.dump(std::cout);

//        alloc.store("alloc.mma1");
    }
    catch (MemoriaThrowable& ex) {
        ex.dump(std::cout);
    }
}
