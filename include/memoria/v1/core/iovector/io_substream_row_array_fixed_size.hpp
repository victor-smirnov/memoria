
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
class IORowwiseFixedSizeArraySubstreamImpl: public IORowwiseFixedSizeArraySubstream {

    uint8_t* data_buffer_{};
    int32_t size_{};
    int32_t capacity_{};

public:
    IORowwiseFixedSizeArraySubstreamImpl(): IORowwiseFixedSizeArraySubstreamImpl(64)
    {}

    IORowwiseFixedSizeArraySubstreamImpl(int32_t initial_capacity)
    {
        data_buffer_  = allocate_system<uint8_t>(initial_capacity * Columns).release();
        size_         = 0;
        capacity_     = initial_capacity;
    }

    virtual ~IORowwiseFixedSizeArraySubstreamImpl() noexcept
    {
        free_system(data_buffer_);
    }

    int32_t columns() const
    {
        return Columns;
    }

    const std::type_info& content_type() const
    {
        return typeid(Value);
    }

    uint8_t* reserve(int32_t rows)
    {
        int32_t head = size_ * sizeof(Value) * Columns;

        if (MMA1_UNLIKELY(size_ + rows > capacity_)) {
            ensure(rows);
        }

        size_ += rows;

        return data_buffer_ + head;
    }

    uint8_t* reserve(int32_t rows, uint64_t* nulls_bitmap) {
        return reserve(rows);
    }

    uint8_t* ensure(int32_t capacity) final
    {
        if (size_ + capacity > capacity_)
        {
            int32_t target_capacity = capacity_;

            while (size_ + capacity > target_capacity)
            {
                target_capacity *= 2;
            }

            uint8_t* new_data_buffer = allocate_system<uint8_t>(target_capacity).release();
            std::memcpy(new_data_buffer, data_buffer_, size_ * sizeof(Value) * Columns);

            free_system(data_buffer_);

            data_buffer_ = new_data_buffer;
            capacity_    = target_capacity;
        }

        return data_buffer_;
    }

    virtual uint8_t* reserve(int32_t rows, int32_t* lengths) {
        return reserve(rows);
    }

    virtual uint8_t* reserve(int32_t rows, int32_t* lengths, uint64_t* nulls_bitmap) {
        return reserve(rows);
    }


    void reset()
    {
        size_ = 0;
    }

    uint8_t* select(int32_t row_idx) const
    {
        return data_buffer_ + row_idx * sizeof(Value) * Columns;
    }

    virtual void reindex() {}
};


}}}
