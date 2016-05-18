
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

#include <memoria/v1/core/tools/strings/string.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

#include "int64_codec.hpp"
#include "uint64_codec.hpp"

namespace memoria {
namespace v1 {

template <typename> class ValueCodec;


template <typename V>
class PrimitiveTypeCodec {
    using Codec = IfThenElse<
            std::is_signed<V>::value,
            ValueCodec<int64_t>,
            ValueCodec<uint64_t>
    >;

    Codec codec_;

public:
    using BufferType    = typename Codec::BufferType;
    using T             = BufferType;
    using VV            = typename Codec::V;

    using ValuePtr      = typename Codec::ValuePtr;

    static constexpr Int BitsPerOffset  = Codec::BitsPerOffset;
    static constexpr Int ElementSize    = Codec::ElementSize;

    auto describe(const T* buffer, size_t idx)
    {
        return codec_.describe(buffer, idx);
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
        return codec_.length(buffer, idx, limit);
    }

    size_t length(V value) const
    {
        return codec_.length(value);
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return decode(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        VV value0;
        auto len = codec_.decode(buffer, value0, idx);
        value = value0;
        return len;
    }

    size_t encode(T* buffer, const ValuePtr& value, size_t idx) const
    {
        return codec_.encode(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx, size_t limit) const
    {
        return codec_.encode(buffer, value, idx);
    }

    size_t encode(T* buffer, V value, size_t idx) const
    {
        return codec_.encode(buffer, value, idx);
    }

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        codec_.move(buffer, from, to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        codec_.copy(src, from, tgt, to, size);
    }
};

template <>
class ValueCodec<Byte>: public PrimitiveTypeCodec<Byte> {};

template <>
class ValueCodec<Char>: public PrimitiveTypeCodec<Char> {};

template <>
class ValueCodec<UByte>: public PrimitiveTypeCodec<UByte> {};

template <>
class ValueCodec<Short>: public PrimitiveTypeCodec<Short> {};

template <>
class ValueCodec<UShort>: public PrimitiveTypeCodec<UShort> {};

template <>
class ValueCodec<Int>: public PrimitiveTypeCodec<Int> {};

template <>
class ValueCodec<UInt>: public PrimitiveTypeCodec<UInt> {};

template <>
class ValueCodec<bool>: public PrimitiveTypeCodec<bool> {};

}}
