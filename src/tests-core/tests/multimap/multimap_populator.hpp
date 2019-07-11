
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

#pragma once

#include <memoria/v1/api/common/ctr_api_btfl.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/random.hpp>

#include <memory>
#include <tuple>
#include <vector>
#include <exception>

namespace memoria {
namespace v1 {
namespace tests {

template <typename Key, typename Value>
using MapData = std::vector<std::pair<Key, std::vector<Value>>>;

template <
        typename Key,
        typename Value,
        template <typename> class KeySubstreamT = io::IOColumnwiseFixedSizeArraySubstream,
        template <typename> class ValueSubstreamT = io::IORowwiseFixedSizeArraySubstream
>
class MultimapTestRandomBufferPopulator: public io::IOVectorProducer {
    const size_t mean_value_size_;
    Key key_cnt_{1};
    Value vv_{};
    uint64_t total_{};
    uint64_t total_max_;
    int32_t entries_per_batch_;

    MapData<Key, Value> map_data_;

public:
    MultimapTestRandomBufferPopulator(size_t mean_value_size, uint64_t total_max, Key start_key = 1, int32_t entries_per_batch = 100):
        mean_value_size_(mean_value_size),
        key_cnt_(start_key),
        total_max_(total_max),
        entries_per_batch_(entries_per_batch)
    {}

    virtual bool populate(io::IOVector& buffer)
    {
        auto& seq = buffer.symbol_sequence();
        auto& s0 = io::substream_cast<KeySubstreamT<Key>>(buffer.substream(0));
        auto& s1 = io::substream_cast<ValueSubstreamT<Value>>(buffer.substream(1));

        for (int r = 0; r < entries_per_batch_; r++)
        {
            int32_t len = getRandomG(mean_value_size_ * 2);
            seq.append(0, 1);
            s0.append(0, key_cnt_);

            std::pair<Key, std::vector<Value>> entry {key_cnt_, std::vector<Value>(len)};

            total_ += sizeof(Key);

            if (len > 0)
            {
                seq.append(1, len);
                Value* val = s1.reserve(len);

                for (int c = 0; c < len; c++)
                {
                    *(val + c) = c;
                    entry.second[c] = c;
                }

                total_ += len * sizeof(Value);
            }

            map_data_.emplace_back(std::move(entry));

            key_cnt_++;
        }

        return total_ >= total_max_;
    }

    const MapData<Key, Value>& map_data() const {
        return map_data_;
    }

    uint64_t total() const {
        return total_;
    }
};


    
}}}
