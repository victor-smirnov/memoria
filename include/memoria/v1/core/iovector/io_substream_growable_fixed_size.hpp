
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

#include <memoria/v1/core/iovector/io_substream_base.hpp>

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


class IOSubstreamFixedSizeGrowable: public IOSubstream {

public:
    IOSubstreamFixedSizeGrowable(): IOSubstreamFixedSizeGrowable(64)
    {}

    IOSubstreamFixedSizeGrowable(int32_t initial_capacity)
    {
        data_buffer_size_ = initial_capacity;        
        data_buffer_      = allocate_system<uint8_t>(initial_capacity).release();
    }

    virtual ~IOSubstreamFixedSizeGrowable() noexcept {
        free_system(data_buffer_);
    }


    uint8_t* reserve(int32_t data_size, int32_t values)
    {
        int32_t last_data_size = data_size_;

        if (MMA1_UNLIKELY(data_size_ + data_size > data_buffer_size_)) {
            enlarge(data_size);
        }

        size_ += values;
        data_size_ += data_size;

        return data_buffer_ + last_data_size;
    }

    uint8_t* reserve(int32_t data_size, int32_t values, uint64_t* nulls_bitmap) {
        return reserve(data_size, values);
    }

    virtual bool is_static() const {
        return false;
    }

    uint8_t* enlarge(int32_t required) final
    {
        if (data_size_ + required > data_buffer_size_)
        {
            int32_t db_size = data_buffer_size_;

            while (data_size_ + required > db_size)
            {
                db_size *= 2;
            }

            uint8_t* new_data_buffer = allocate_system<uint8_t>(db_size).release();
            std::memcpy(new_data_buffer, data_buffer_, data_buffer_size_);

            free_system(data_buffer_);

            data_buffer_ = new_data_buffer;
            data_buffer_size_ = db_size;
        }

        return data_buffer_;
    }

    virtual void reset() {
        data_size_ = 0;
        size_ = 0;
    }

protected:
    void range_check(int32_t pos, int32_t size) const
    {
        if (MMA1_UNLIKELY(pos + size > size_))
        {
            MMA1_THROW(BoundsException()) << fmt::format_ex(u"IOStreamUnalignedNative range check: pos={}, size={}, stream size={}", pos, size, size_);
        }
    }
};


template <typename T>
class IOSubstreamTypedFixedSizeGrowable: public IOSubstreamFixedSizeGrowable {

    static_assert(sizeof(T) > 8 ? (sizeof(T) % 4 == 0) : true, "");

public:
    IOSubstreamTypedFixedSizeGrowable(): IOSubstreamTypedFixedSizeGrowable(64)
    {}

    IOSubstreamTypedFixedSizeGrowable(int32_t initial_capacity):
        IOSubstreamFixedSizeGrowable(initial_capacity)
    {
        value_size_       = sizeof(T);
        alignment_        = alignof(T);
        alignment_mask_   = (1 << (Log2(alignof(T)) - 1)) - 1;
    }

    virtual const std::type_info& type() const {
        return typeid(T);
    }

    virtual uint8_t* select(int32_t idx) const {
        return data_buffer_ + idx * sizeof(T);
    }
};



}}}
