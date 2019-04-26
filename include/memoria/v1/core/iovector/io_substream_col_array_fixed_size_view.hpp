
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


#include <type_traits>
#include <cstring>

namespace memoria {
namespace v1 {
namespace io {


template <typename Value, int32_t Columns>
class IOColumnwiseFixedSizeArraySubstreamViewImpl final: public IOColumnwiseFixedSizeArraySubstream<Value> {
    FixedSizeArrayColumnMetadata<Value> columns_[Columns]{};

public:
    IOColumnwiseFixedSizeArraySubstreamViewImpl()
    {

    }

    virtual ~IOColumnwiseFixedSizeArraySubstreamViewImpl() noexcept {}


    void configure(FixedSizeArrayColumnMetadata<Value>* columns) noexcept
    {
        for (int32_t c = 0; c < Columns; c++)
        {
            columns_[c] = columns[c];
        }
    }

    FixedSizeArrayColumnMetadata<Value> describe(int32_t column)
    {
        return columns_[column];
    }

    FixedSizeArrayColumnMetadata<const Value> describe(int32_t column) const
    {
        auto& col = columns_[column];
        return FixedSizeArrayColumnMetadata<const Value>{
            col.data_buffer,
            col.capacity,
            col.size
        };
    }

    int32_t columns() const
    {
        return Columns;
    }

    void append(int32_t column, const Value& value) {
        MMA1_THROW(UnsupportedOperationException());
    }


    Value* reserve(int32_t column, int32_t values)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    Value* ensure(int32_t column, int32_t required) final
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    void reset()
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    const Value* select(int32_t column, int32_t idx) const
    {
        return columns_[column].data_buffer + idx;
    }

    Value* select(int32_t column, int32_t idx)
    {
        MMA1_THROW(UnsupportedOperationException());
    }



    virtual void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        auto& tgt_substream = substream_cast<IOColumnwiseFixedSizeArraySubstream<Value>>(target);

        for (int32_t col = 0; col < Columns; col++)
        {
            const Value* src = select(col, start);
            Value* tgt = tgt_substream.reserve(col, length);

            std::memcpy(tgt, src, sizeof(Value) * length);
        }
    }

    virtual void reindex() {}
};


}}}
