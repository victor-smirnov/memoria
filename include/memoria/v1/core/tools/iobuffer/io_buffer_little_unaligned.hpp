
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
class IOBuffer<ByteOrder::LITTLE, MemoryAccess::UNALIGNED>: public IOBufferBase {
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
            *T2T<Short*>(array_ + pos_) = v;

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
        Short v = *T2T<const Short*>(array_ + pos_);

        pos_ += 2;

        return v;
    }


    bool put(UShort v)
    {
        if (has_capacity(2))
        {
            *T2T<UShort*>(array_ + pos_) = v;

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
        Short v = *T2T<const UShort*>(array_ + pos_);

        pos_ += 2;

        return v;
    }

    bool put(Int v)
    {
        if (has_capacity(4))
        {
            *T2T<Int*>(array_ + pos_) = v;

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
        Int v = *T2T<const Int*>(array_ + pos_);

        pos_ += 4;

        return v;
    }


    bool put(UInt v)
    {
        if (has_capacity(4))
        {
            *T2T<UInt*>(array_ + pos_) = v;

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
        UInt v = *T2T<const UInt*>(array_ + pos_);

        pos_ += 4;

        return v;
    }

    bool put(BigInt v)
    {
        if (has_capacity(8))
        {
            *T2T<BigInt*>(array_ + pos_) = v;

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
        BigInt v = *T2T<const BigInt*>(array_ + pos_);

        pos_ += 8;

        return v;
    }

    bool put(UBigInt v)
    {
        if (has_capacity(8))
        {
            *T2T<UBigInt*>(array_ + pos_) = v;

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

        UBigInt v = *T2T<const UBigInt*>(array_ + pos_);

        pos_ += 8;

        return v;
    }

    bool put(float v)
    {
        if (has_capacity(4))
        {
            *T2T<float*>(array_ + pos_) = v;

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
        float v = *T2T<const float*>(array_ + pos_);

        pos_ += 4;

        return v;
    }


    bool put(double v)
    {
        if (has_capacity(8))
        {
            *T2T<double*>(array_ + pos_) = v;

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
        double v = *T2T<const double*>(array_ + pos_);

        pos_ += 8;

        return static_cast<double>(v);
    }


};





}
}
