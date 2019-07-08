
// Copyright 2017 Victor Smirnov
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
#include <exception>

namespace memoria {
namespace v1 {


template <typename Key, typename Value>
class MultimapRandomBufferPopulator: public io::IOVectorProducer {
    const size_t mean_value_size_;
    Key key_cnt_{1};
    Value vv_{};
    uint64_t total_{};
    uint64_t total_max_;
    int32_t entries_per_batch_;
public:
    MultimapRandomBufferPopulator(size_t mean_value_size, uint64_t total_max, int32_t entries_per_batch = 100):
        mean_value_size_(mean_value_size),
        total_max_(total_max),
        entries_per_batch_(entries_per_batch)
    {}

    virtual bool populate(io::IOVector& buffer)
    {
        auto& seq = buffer.symbol_sequence();
        auto& s0 = io::substream_cast<io::IOColumnwiseFixedSizeArraySubstream<Key>>(buffer.substream(0));
        auto& s1 = io::substream_cast<io::IORowwiseFixedSizeArraySubstream<Value>>(buffer.substream(1));

        for (int r = 0; r < entries_per_batch_; r++)
        {
            int32_t len = getRandomG(mean_value_size_ * 2) + 1;
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


    
}}
