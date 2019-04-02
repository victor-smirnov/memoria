
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


template <typename PkdStruct>
class IOColumnwiseVLenArraySubstreamViewImpl final: public IOColumnwiseVLenArraySubstream<typename PkdStruct::Value> {

    PkdStruct* array_{};

public:
    IOColumnwiseVLenArraySubstreamViewImpl()
    {

    }

    virtual ~IOColumnwiseVLenArraySubstreamViewImpl() noexcept {}


    void configure(PkdStruct* array) noexcept
    {
        array_ = array;
    }

    ConstVLenArrayColumnMetadata describe(int32_t column) const
    {
        auto meta = array_->metadata();
        return ConstVLenArrayColumnMetadata{
            array_->values(column),
            meta->data_size(column),
            meta->data_size(column),
            meta->size()
        };
    }

    VLenArrayColumnMetadata describe(int32_t column)
    {
        MMA1_THROW(UnsupportedOperationException());
    }


    int32_t columns() const
    {
        return PkdStruct::Blocks;
    }


    uint8_t* ensure(int32_t column, int32_t required)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual uint8_t* reserve(int32_t column, int32_t size, const int32_t* lengths) {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual uint8_t* reserve(int32_t column, int32_t size, const int32_t* lengths, const uint64_t* nulls_bitmap) {
        MMA1_THROW(UnsupportedOperationException());
    }

    void reset()
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    const uint8_t* select(int32_t column, int32_t idx) const
    {
        return array_->value_ptr(column, idx);
    }

    uint8_t* select(int32_t column, int32_t idx)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    ConstVLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx) const
    {
        auto meta = array_->metadata();
        auto* value = array_->value_ptr(column, idx);
        return ConstVLenArrayColumnMetadata{
            value,
            meta->data_size(column),
            meta->data_size(column),
            meta->size()
        };
    }

    VLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual void reindex() {}
};


}}}
