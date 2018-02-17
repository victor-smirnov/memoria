
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/bytes/bytes_codec.hpp>
#include <memoria/v1/core/bignum/int64_codec.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/strings/string_codec.hpp>

#include "io_buffer_base.hpp"


#include <limits>

//#include <malloc.h>

namespace memoria {
namespace v1 {

template <>
class IOBuffer<ByteOrder::LITTLE, MemoryAccess::MMA_ALIGNED>: public IOBufferBase {
    using Base = IOBufferBase;

public:

    IOBuffer(): Base()
    {}

    IOBuffer(size_t length): Base(length)
    {}

    IOBuffer(uint8_t* data, size_t length): Base(data, length)
    {}

    IOBuffer(uint8_t* data, size_t pos, size_t length): Base(data, pos, length)
    {}

    IOBuffer(const IOBuffer&) = delete;

    IOBuffer(IOBuffer&& other): Base(std::move(other))
    {}

    using Base::put;

    bool put(int16_t v)
    {
        if (has_capacity(2))
        {
            array_[pos_]      = v & 0xFF;
            array_[pos_ + 1]  = (v >> 8) & 0xFF;

            pos_ += 2;

            return true;
        }
        else {
            return false;
        }
    }

    int16_t getShort()
    {
        assertRange(2, "getShort()");
        int16_t v = 0;

        v = array_[pos_];
        v |= ((int16_t)array_[pos_ + 1]) << 8;

        pos_ += 2;

        return v;
    }


    bool put(uint16_t v)
    {
        if (has_capacity(2))
        {
            array_[pos_] = v & 0xFF;
            array_[pos_ + 1] = (v >> 8) & 0xFF;

            pos_ += 2;

            return true;
        }
        else {
            return false;
        }
    }

    uint16_t getUShort()
    {
        assertRange(2, "getShort()");
        int16_t v = 0;

        v = array_[pos_];
        v |= ((uint16_t)array_[pos_ + 1]) << 8;

        pos_ += 2;

        return v;
    }

    bool put(int32_t v)
    {
        if (has_capacity(4))
        {
            array_[pos_] = v & 0xFF;
            array_[pos_ + 1] = (v >> 8) & 0xFF;
            array_[pos_ + 2] = (v >> 16) & 0xFF;
            array_[pos_ + 3] = (v >> 24) & 0xFF;

            pos_ += 4;

            return true;
        }
        else {
            return false;
        }
    }

    int32_t getInt()
    {
        assertRange(4, "getInt()");
        int32_t v = 0;

        v = array_[pos_];
        v |= ((int32_t)array_[pos_ + 1]) << 8;
        v |= ((int32_t)array_[pos_ + 2]) << 16;
        v |= ((int32_t)array_[pos_ + 3]) << 24;

        pos_ += 4;

        return v;
    }


    bool put(uint32_t v)
    {
        if (has_capacity(4))
        {
            array_[pos_] = v & 0xFF;
            array_[pos_ + 1] = (v >> 8) & 0xFF;
            array_[pos_ + 2] = (v >> 16) & 0xFF;
            array_[pos_ + 3] = (v >> 24) & 0xFF;

            pos_ += 4;

            return true;
        }
        else {
            return false;
        }
    }

    uint32_t getUInt32()
    {
        assertRange(4, "getUInt32()");
        uint32_t v = 0;

        v = array_[pos_];
        v |= ((uint32_t)array_[pos_ + 1]) << 8;
        v |= ((uint32_t)array_[pos_ + 2]) << 16;
        v |= ((uint32_t)array_[pos_ + 3]) << 24;

        pos_ += 4;

        return v;
    }

    bool put(int64_t v)
    {
        if (has_capacity(8))
        {
            array_[pos_] = v & 0xFF;
            array_[pos_ + 1] = (v >> 8) & 0xFF;
            array_[pos_ + 2] = (v >> 16) & 0xFF;
            array_[pos_ + 3] = (v >> 24) & 0xFF;
            array_[pos_ + 4] = (v >> 32) & 0xFF;
            array_[pos_ + 5] = (v >> 40) & 0xFF;
            array_[pos_ + 6] = (v >> 48) & 0xFF;
            array_[pos_ + 7] = (v >> 56) & 0xFF;

            pos_ += 8;

            return true;
        }
        else {
            return false;
        }
    }

    int64_t getInt64()
    {
        assertRange(8, "getInt64()");
        int64_t v = 0;

        v = array_[pos_];
        v |= ((int64_t)array_[pos_ + 1]) << 8;
        v |= ((int64_t)array_[pos_ + 2]) << 16;
        v |= ((int64_t)array_[pos_ + 3]) << 24;
        v |= ((int64_t)array_[pos_ + 4]) << 32;
        v |= ((int64_t)array_[pos_ + 5]) << 40;
        v |= ((int64_t)array_[pos_ + 6]) << 48;
        v |= ((int64_t)array_[pos_ + 7]) << 56;

        pos_ += 8;

        return v;
    }

    bool put(uint64_t v)
    {
        if (has_capacity(8))
        {
            array_[pos_] = v & 0xFF;
            array_[pos_ + 1] = (v >> 8) & 0xFF;
            array_[pos_ + 2] = (v >> 16) & 0xFF;
            array_[pos_ + 3] = (v >> 24) & 0xFF;
            array_[pos_ + 4] = (v >> 32) & 0xFF;
            array_[pos_ + 5] = (v >> 40) & 0xFF;
            array_[pos_ + 6] = (v >> 48) & 0xFF;
            array_[pos_ + 7] = (v >> 56) & 0xFF;

            pos_ += 8;

            return true;
        }
        else {
            return false;
        }
    }

    uint64_t getUInt64()
    {
        assertRange(8, "getUInt64()");

        uint64_t v = 0;
        v = array_[pos_];
        v |= ((uint64_t)array_[pos_ + 1]) << 8;
        v |= ((uint64_t)array_[pos_ + 2]) << 16;
        v |= ((uint64_t)array_[pos_ + 3]) << 24;
        v |= ((uint64_t)array_[pos_ + 4]) << 32;
        v |= ((uint64_t)array_[pos_ + 5]) << 40;
        v |= ((uint64_t)array_[pos_ + 6]) << 48;
        v |= ((uint64_t)array_[pos_ + 7]) << 56;

        pos_ += 8;

        return v;
    }

    bool put(float f)
    {
        if (has_capacity(4))
        {
            int32_t v = static_cast<int32_t>(f);

            array_[pos_] = v & 0xFF;
            array_[pos_ + 1] = (v >> 8) & 0xFF;
            array_[pos_ + 2] = (v >> 16) & 0xFF;
            array_[pos_ + 3] = (v >> 24) & 0xFF;

            pos_ += 4;

            return true;
        }
        else {
            return false;
        }
    }

    float getFloat()
    {
        assertRange(4, "getFloat()");
        int32_t v = 0;

        v = array_[pos_];
        v |= ((int32_t)array_[pos_ + 1]) << 8;
        v |= ((int32_t)array_[pos_ + 2]) << 16;
        v |= ((int32_t)array_[pos_ + 3]) << 24;

        pos_ += 4;

        return static_cast<float>(v);
    }


    bool put(double f)
    {
        if (has_capacity(8))
        {
            int64_t v = static_cast<int64_t>(f);

            array_[pos_] = v & 0xFF;
            array_[pos_ + 1] = (v >> 8) & 0xFF;
            array_[pos_ + 2] = (v >> 16) & 0xFF;
            array_[pos_ + 3] = (v >> 24) & 0xFF;
            array_[pos_ + 4] = (v >> 32) & 0xFF;
            array_[pos_ + 5] = (v >> 40) & 0xFF;
            array_[pos_ + 6] = (v >> 48) & 0xFF;
            array_[pos_ + 7] = (v >> 56) & 0xFF;

            pos_ += 8;

            return true;
        }
        else {
            return false;
        }
    }

    double getDouble()
    {
        assertRange(8, "getDouble()");
        int64_t v = 0;

        v = array_[pos_];
        v |= ((int64_t)array_[pos_ + 1]) << 8;
        v |= ((int64_t)array_[pos_ + 2]) << 16;
        v |= ((int64_t)array_[pos_ + 3]) << 24;
        v |= ((int64_t)array_[pos_ + 4]) << 32;
        v |= ((int64_t)array_[pos_ + 5]) << 40;
        v |= ((int64_t)array_[pos_ + 6]) << 48;
        v |= ((int64_t)array_[pos_ + 7]) << 56;

        pos_ += 8;

        return static_cast<double>(v);
    }
};






}
}
