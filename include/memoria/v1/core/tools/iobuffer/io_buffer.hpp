
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

#include "io_buffer_little_aligned.hpp"
#include "io_buffer_little_unaligned.hpp"




#include <limits>

#include <malloc.h>

namespace memoria {
namespace v1 {

using DefaultIOBuffer = IOBuffer<ByteOrder::LITTLE, MemoryAccess::MMA_UNALIGNED>;
using AlignedIOBuffer = IOBuffer<ByteOrder::LITTLE, MemoryAccess::MMA_ALIGNED>;


template <typename T>
struct IOBufferAdapterBase {
    template <typename IOBuffer>
    static bool put(IOBuffer& buffer, const T& value) {
        return buffer.put(value);
    }
};

template <>
struct IOBufferAdapter<Char>: IOBufferAdapterBase<Char> {
    template <typename IOBuffer>
    static Char get(IOBuffer& buffer) {
        return buffer.getChar();
    }
};

template <>
struct IOBufferAdapter<Byte>: IOBufferAdapterBase<Byte> {
    template <typename IOBuffer>
    static Byte get(IOBuffer& buffer) {
        return buffer.getByte();
    }
};

template <>
struct IOBufferAdapter<UByte>: IOBufferAdapterBase<UByte> {
    template <typename IOBuffer>
    static UByte get(IOBuffer& buffer) {
        return buffer.getUByte();
    }
};

template <>
struct IOBufferAdapter<Short>: IOBufferAdapterBase<Short> {
    template <typename IOBuffer>
    static Short get(IOBuffer& buffer) {
        return buffer.getShort();
    }
};

template <>
struct IOBufferAdapter<UShort>: IOBufferAdapterBase<UShort> {
    template <typename IOBuffer>
    static UShort get(IOBuffer& buffer) {
        return buffer.getUShort();
    }
};

template <>
struct IOBufferAdapter<Int>: IOBufferAdapterBase<Int> {
    template <typename IOBuffer>
    static Int get(IOBuffer& buffer) {
        return buffer.getInt();
    }
};

template <>
struct IOBufferAdapter<UInt>: IOBufferAdapterBase<UInt> {
    template <typename IOBuffer>
    static UInt get(IOBuffer& buffer) {
        return buffer.getUInt();
    }
};


template <>
struct IOBufferAdapter<BigInt>: IOBufferAdapterBase<BigInt> {
    template <typename IOBuffer>
    static BigInt get(IOBuffer& buffer) {
        return buffer.getBigInt();
    }
};

template <>
struct IOBufferAdapter<UBigInt>: IOBufferAdapterBase<UBigInt> {
    template <typename IOBuffer>
    static UBigInt get(IOBuffer& buffer) {
        return buffer.getUBigInt();
    }
};

template <>
struct IOBufferAdapter<float>: IOBufferAdapterBase<float> {
    template <typename IOBuffer>
    static float get(IOBuffer& buffer) {
        return buffer.getFloat();
    }
};


template <>
struct IOBufferAdapter<double>: IOBufferAdapterBase<double> {
    template <typename IOBuffer>
    static double get(IOBuffer& buffer) {
        return buffer.getDouble();
    }
};

template <>
struct IOBufferAdapter<bool>: IOBufferAdapterBase<bool> {
    template <typename IOBuffer>
    static double get(IOBuffer& buffer) {
        return buffer.getBool();
    }
};

template <>
struct IOBufferAdapter<Bytes>: IOBufferAdapterBase<Bytes> {
    template <typename IOBuffer>
    static Bytes get(IOBuffer& buffer) {
        return buffer.getBytes();
    }
};

template <>
struct IOBufferAdapter<String>: IOBufferAdapterBase<String> {
    template <typename IOBuffer>
    static String get(IOBuffer& buffer) {
        return buffer.getString();
    }
};

template <>
struct IOBufferAdapter<UUID> {

    template <typename IOBuffer>
    static bool put(IOBuffer& buffer, const UUID& value)
    {
        if (buffer.put(value.hi()))
        {
            if (buffer.put(value.lo())) {
                return true;
            }
        }

        return false;
    }

    template <typename IOBuffer>
    static UUID get(IOBuffer& buffer)
    {
        return UUID(buffer.getUBigInt(), buffer.getUBigInt());
    }
};



}
}
