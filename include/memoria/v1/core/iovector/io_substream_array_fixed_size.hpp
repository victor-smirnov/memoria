
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

#include <memoria/v1/core/iovector/io_substream_array_base.hpp>

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
class IOColumnwiseArraySubstreamFixedSize: public IOColumnwiseArraySubstream {
    ArrayColumnMetadata columns_[Columns]{};

public:
    IOColumnwiseArraySubstreamFixedSize(): IOColumnwiseArraySubstreamFixedSize(64)
    {}

    IOColumnwiseArraySubstreamFixedSize(int32_t initial_capacity)
    {
        for (int32_t c = 0; c < Columns; c++)
        {
            columns_[c].data_buffer  = allocate_system<uint8_t>(initial_capacity).release();
            columns_[c].size         = 0;
            columns_[c].data_size    = 0;
            columns_[c].data_buffer_size = initial_capacity;
        }
    }

    virtual ~IOColumnwiseArraySubstreamFixedSize() noexcept
    {
        for (int32_t c = 0; c < Columns; c++)
        {
            free_system(columns_[c].data_buffer);
        }
    }

    ArrayColumnMetadata describe(int32_t column) const
    {
        return columns_[column];
    }

    int32_t columns() const
    {
        return Columns;
    }

    const std::type_info& content_type() const
    {
        return typeid(Value);
    }

    uint8_t* reserve(int32_t column, int32_t data_size, int32_t values)
    {
        auto& col = columns_[column];

        int32_t last_data_size = col.data_size;

        if (MMA1_UNLIKELY(col.data_size + data_size > col.data_buffer_size)) {
            enlarge(column, data_size);
        }

        col.size += values;
        col.data_size += data_size;

        return col.data_buffer + last_data_size;
    }

    uint8_t* reserve(int32_t column, int32_t data_size, int32_t values, uint64_t* nulls_bitmap) {
        return reserve(column, data_size, values);
    }

    uint8_t* enlarge(int32_t column, int32_t required) final
    {
        auto& col = columns_[column];

        if (col.data_size + required > col.data_buffer_size)
        {
            int32_t db_size = col.data_buffer_size;

            while (col.data_size + required > db_size)
            {
                db_size *= 2;
            }

            uint8_t* new_data_buffer = allocate_system<uint8_t>(db_size).release();
            std::memcpy(new_data_buffer, col.data_buffer, col.data_buffer_size);

            free_system(col.data_buffer);

            col.data_buffer      = new_data_buffer;
            col.data_buffer_size = db_size;
        }

        return col.data_buffer;
    }

    virtual uint8_t* reserve(int32_t column, int32_t data_size, int32_t values, int32_t* lengths) {
        return reserve(column, data_size, values);
    }

    virtual uint8_t* reserve(int32_t column, int32_t data_size, int32_t values, int32_t* lengths, uint64_t* nulls_bitmap) {
        return reserve(column, data_size, values);
    }


    void reset()
    {
        for (int32_t c = 0; c < Columns; c++)
        {
            columns_[c].size = 0;
            columns_[c].data_size = 0;
        }
    }

    uint8_t* select(int32_t column, int32_t idx) const
    {
        return columns_[column].data_buffer + idx * sizeof(Value);
    }

    ArrayColumnMetadata select_and_describe(int32_t column, int32_t idx) const
    {
        auto& col = columns_[column];

        int32_t offs = idx * sizeof(Value);
        return ArrayColumnMetadata{col.data_buffer + offs, col.data_size - offs, col.data_buffer_size - offs, col.size};
    }

    virtual void reindex() {}

protected:
    void init(void* ptr) {
        MMA1_THROW(UnsupportedOperationException());
    }
};


}}}
