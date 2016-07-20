
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
#include <memoria/v1/core/tools/bignum/int64_codec.hpp>
#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/uuid.hpp>


#include "io_buffer_base.hpp"


#include <limits>

#include <malloc.h>

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

    IOBuffer(UByte* data, size_t length): Base(data, length)
    {}

    IOBuffer(UByte* data, size_t pos, size_t length): Base(data, pos, length)
    {}

    IOBuffer(const IOBuffer&) = delete;

    IOBuffer(IOBuffer&& other): Base(std::move(other))
    {}

    using Base::put;

    bool put(Short v)
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

    Short getShort()
    {
        assertRange(2, "getShort()");
        Short v = 0;

        v = array_[pos_];
        v |= ((Short)array_[pos_ + 1]) << 8;

        pos_ += 2;

        return v;
    }


    bool put(UShort v)
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

    UShort getUShort()
    {
        assertRange(2, "getShort()");
        Short v = 0;

        v = array_[pos_];
        v |= ((UShort)array_[pos_ + 1]) << 8;

        pos_ += 2;

        return v;
    }

    bool put(Int v)
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

    Int getInt()
    {
        assertRange(4, "getInt()");
        Int v = 0;

        v = array_[pos_];
        v |= ((Int)array_[pos_ + 1]) << 8;
        v |= ((Int)array_[pos_ + 2]) << 16;
        v |= ((Int)array_[pos_ + 3]) << 24;

        pos_ += 4;

        return v;
    }


    bool put(UInt v)
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

    UInt getUInt()
    {
        assertRange(4, "getUInt()");
        UInt v = 0;

        v = array_[pos_];
        v |= ((UInt)array_[pos_ + 1]) << 8;
        v |= ((UInt)array_[pos_ + 2]) << 16;
        v |= ((UInt)array_[pos_ + 3]) << 24;

        pos_ += 4;

        return v;
    }

    bool put(BigInt v)
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

    BigInt getBigInt()
    {
        assertRange(8, "getBigInt()");
        BigInt v = 0;

        v = array_[pos_];
        v |= ((BigInt)array_[pos_ + 1]) << 8;
        v |= ((BigInt)array_[pos_ + 2]) << 16;
        v |= ((BigInt)array_[pos_ + 3]) << 24;
        v |= ((BigInt)array_[pos_ + 4]) << 32;
        v |= ((BigInt)array_[pos_ + 5]) << 40;
        v |= ((BigInt)array_[pos_ + 6]) << 48;
        v |= ((BigInt)array_[pos_ + 7]) << 56;

        pos_ += 8;

        return v;
    }

    bool put(UBigInt v)
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

    UBigInt getUBigInt()
    {
        assertRange(8, "getUBigInt()");

        UBigInt v = 0;
        v = array_[pos_];
        v |= ((UBigInt)array_[pos_ + 1]) << 8;
        v |= ((UBigInt)array_[pos_ + 2]) << 16;
        v |= ((UBigInt)array_[pos_ + 3]) << 24;
        v |= ((UBigInt)array_[pos_ + 4]) << 32;
        v |= ((UBigInt)array_[pos_ + 5]) << 40;
        v |= ((UBigInt)array_[pos_ + 6]) << 48;
        v |= ((UBigInt)array_[pos_ + 7]) << 56;

        pos_ += 8;

        return v;
    }

    bool put(float f)
    {
        if (has_capacity(4))
        {
            Int v = static_cast<Int>(f);

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
        Int v = 0;

        v = array_[pos_];
        v |= ((Int)array_[pos_ + 1]) << 8;
        v |= ((Int)array_[pos_ + 2]) << 16;
        v |= ((Int)array_[pos_ + 3]) << 24;

        pos_ += 4;

        return static_cast<float>(v);
    }


    bool put(double f)
    {
        if (has_capacity(8))
        {
            BigInt v = static_cast<BigInt>(f);

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
        BigInt v = 0;

        v = array_[pos_];
        v |= ((BigInt)array_[pos_ + 1]) << 8;
        v |= ((BigInt)array_[pos_ + 2]) << 16;
        v |= ((BigInt)array_[pos_ + 3]) << 24;
        v |= ((BigInt)array_[pos_ + 4]) << 32;
        v |= ((BigInt)array_[pos_ + 5]) << 40;
        v |= ((BigInt)array_[pos_ + 6]) << 48;
        v |= ((BigInt)array_[pos_ + 7]) << 56;

        pos_ += 8;

        return static_cast<double>(v);
    }
};






}
}
