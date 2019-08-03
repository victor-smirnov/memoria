
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

#include <memoria/v1/core/iovector/io_substream_array_fixed_size_base.hpp>

#include <memoria/v1/core/types/type2type.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>

#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

#include <memoria/v1/core/tools/spliterator.hpp>

#include <type_traits>
#include <cstring>

namespace memoria {
namespace v1 {
namespace io {

template <typename Value, int32_t Columns>
class IORowwiseFixedSizeArraySubstreamViewImpl: public IORowwiseFixedSizeArraySubstream<Value> {

    const Value* data_buffer_{};
    int32_t size_{};

    static constexpr int32_t BUFFER_SIZE = 256;

public:
    IORowwiseFixedSizeArraySubstreamViewImpl()
    {}

    virtual ~IORowwiseFixedSizeArraySubstreamViewImpl() noexcept
    {
    }

    int32_t columns() const
    {
        return Columns;
    }

    Value* reserve(int32_t rows)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    Value* ensure(int32_t capacity) final
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    void reset()
    {
        size_ = 0;
    }

    const Value* select(int32_t row_idx) const
    {
        return data_buffer_ + row_idx * Columns;
    }

    Value* select(int32_t row_idx)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    void reindex() {}



    void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        auto& tgt_substream = substream_cast<
                IORowwiseFixedSizeArraySubstream<Value>
        >(target);

        FixedSizeSpliterator<int32_t, BUFFER_SIZE> splits(length);

        while (splits.has_more())
        {
            auto* dest = tgt_substream.reserve(splits.split_size());
            MemCpyBuffer(this->data_buffer_, dest, splits.split_size() * Columns);
            splits.next_split();
        }
    }

    void configure(const Value* buffer, int32_t size)
    {
        data_buffer_ = buffer;
        size_        = size;
    }
};


}}}