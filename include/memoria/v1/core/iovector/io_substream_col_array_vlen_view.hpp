
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

#include <memoria/v1/core/iovector/io_substream_array_vlen_base.hpp>

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
class IOColumnwiseVLenArraySubstreamViewImpl: public IOColumnwiseVLenArraySubstream {
    VLenArrayColumnMetadata columns_[Columns]{};

public:
    IOColumnwiseVLenArraySubstreamViewImpl()
    {

    }

    virtual ~IOColumnwiseVLenArraySubstreamViewImpl() noexcept {}


    void configure(VLenArrayColumnMetadata* columns) noexcept
    {
        for (int32_t c = 0; c < Columns; c++)
        {
            columns_[c] = columns[c];
        }
    }

    VLenArrayColumnMetadata describe(int32_t column) const
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


    uint8_t* ensure(int32_t column, int32_t required) final
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual uint8_t* reserve(int32_t column, int32_t size, int32_t* lengths) {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual uint8_t* reserve(int32_t column, int32_t size, int32_t* lengths, uint64_t* nulls_bitmap) {
        MMA1_THROW(UnsupportedOperationException());
    }

    void reset()
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    uint8_t* select(int32_t column, int32_t idx) const
    {
        return columns_[column].data_buffer + idx * sizeof(Value);
    }

    VLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx) const
    {
        auto& col = columns_[column];

        int32_t offs = idx * sizeof(Value);
        return VLenArrayColumnMetadata{col.data_buffer + offs, col.data_size - offs, col.data_buffer_size - offs, col.size};
    }

    virtual void reindex() {}
};


}}}
