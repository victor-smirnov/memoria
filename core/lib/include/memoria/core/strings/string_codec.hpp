
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

#include <memoria/core/types.hpp>
#include <memoria/core/bignum/uint64_codec.hpp>
#include <memoria/core/tools/optional.hpp>

#include <memoria/core/strings/u8_string.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

namespace memoria {

template <typename> class ValueCodec;


template<>
class ValueCodec<U8String> {
public:
    using BufferType    = uint8_t;
    using T             = BufferType;
    using V             = U8String;

    using ValuePtr      = ValuePtrT1<BufferType>;

    ValueCodec<uint64_t> size_codec_;

    static const int32_t BitsPerOffset  = 16;
    static const int32_t ElementSize    = 8; // In bits;

    ValuePtr describe(const T* buffer, size_t idx)
    {
        return ValuePtr(buffer + idx, length(buffer, idx, -1ull));
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
        uint64_t len;
        size_t pos = idx;

        pos += size_codec_.decode(buffer, len, idx);

        return pos - idx + len;
    }

    size_t length(const V& value) const
    {
        auto len = value.size();
        return size_codec_.length(len) + len;
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return decode(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        uint64_t len0 = 0;
        size_t pos = idx;
        pos += size_codec_.decode(buffer, len0, idx);

        size_t len = len0;

        V tmp(len, ' ');

        for (size_t c = 0; c < len; c++)
        {
            tmp[c] = buffer[c + pos];
        }

        std::memcpy(tmp.data(), buffer + pos, len);

        std::swap(value, tmp);

        return pos + len - idx;
    }

    size_t encode(T* buffer, const ValuePtr& value, size_t idx) const
    {
        copy(value.addr(), 0, buffer, idx, value.length());
        return value.length();
    }


    size_t encode(T* buffer, const V& value, size_t idx, size_t limit) const
    {
        return encode(buffer, value, idx);
    }

    size_t encode(T* buffer, const V& value, size_t idx) const
    {
        size_t pos = idx;
        pos += size_codec_.encode(buffer, value.size(), pos);

        size_t len = value.size();

        std::memcpy(buffer + pos, value.data(), len);

        return pos + len - idx;
    }

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        CopyBuffer(buffer + from, buffer + to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        CopyBuffer(src + from, tgt + to, size);
    }
};



template<>
class ValueCodec<U8StringView> {
public:
    using BufferType    = uint8_t;
    using T             = BufferType;
    using V             = U8StringView;

    static_assert(sizeof(T) == sizeof(typename V::value_type), "");

    using ValuePtr      = ValuePtrT1<BufferType>;

    ValueCodec<uint64_t> size_codec_;

    static const int32_t BitsPerOffset  = 16;
    static const int32_t ElementSize    = 8; // In bits;

    ValuePtr describe(const T* buffer, size_t idx)
    {
        return ValuePtr(buffer + idx, length(buffer, idx, -1ull));
    }

    size_t length(const T* buffer, size_t idx, size_t limit = -1ull) const
    {
        uint64_t len;
        size_t pos = idx;

        pos += size_codec_.decode(buffer, len, idx);

        return pos - idx + len;
    }

    size_t length(const V& value) const
    {
        auto len = value.size();
        return size_codec_.length(len) + len;
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return decode(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        uint64_t len0 = 0;
        size_t pos = idx;
        pos += size_codec_.decode(buffer, len0, idx);

        size_t len = len0;

        value = V(ptr_cast<std::remove_pointer_t<typename V::const_pointer>>(buffer + pos), len);

        return pos + len - idx;
    }

    size_t encode(T* buffer, const ValuePtr& value, size_t idx) const
    {
        copy(value.addr(), 0, buffer, idx, value.length());
        return value.length();
    }


    size_t encode(T* buffer, const V& value, size_t idx, size_t limit) const
    {
        return encode(buffer, value, idx);
    }

    size_t encode(T* buffer, const V& value, size_t idx) const
    {
        size_t pos = idx;
        pos += size_codec_.encode(buffer, value.size(), pos);

        size_t len = value.size();

        std::memcpy(buffer + pos, value.data(), len);

        return pos + len - idx;
    }

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        CopyBuffer(buffer + from, buffer + to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        CopyBuffer(src + from, tgt + to, size);
    }
};


/*
template<>
class ValueCodec<Optional<String>> {
public:
    using BufferType    = uint8_t;
    using T             = BufferType;
    using V             = String;

    using ValuePtr      = ValuePtrT1<BufferType>;

    ValueCodec<uint64_t> size_codec_;

    static const int32_t BitsPerOffset  = 16;
    static const int32_t ElementSize    = 8; // In bits;

    ValuePtr describe(const T* buffer, size_t idx)
    {
        return ValuePtr(buffer + idx, length(buffer, idx, -1ull));
    }

    size_t length(const T* buffer, size_t idx, size_t limit) const
    {
        uint64_t len;
        size_t pos = idx;

        pos += size_codec_.decode(buffer, len, idx);

        return pos - idx + len;
    }

    size_t length(const V& value) const
    {
        auto len = value.size();
        return size_codec_.length(len) + len;
    }

    size_t decode(const T* buffer, V& value, size_t idx, size_t limit) const
    {
        return decode(buffer, value, idx);
    }

    size_t decode(const T* buffer, V& value, size_t idx) const
    {
        uint64_t len0 = 0;
        size_t pos = idx;
        pos += size_codec_.decode(buffer, len0, idx);

        size_t len = len0;

        V tmp(len, ' ');

        for (size_t c = 0; c < len; c++)
        {
            tmp[c] = buffer[c + pos];
        }

        pos += len;

        value.swap(tmp);

        return pos - idx;
    }

    size_t encode(T* buffer, const ValuePtr& value, size_t idx) const
    {
        copy(value.addr(), 0, buffer, idx, value.length());
        return value.length();
    }


    size_t encode(T* buffer, const V& value, size_t idx, size_t limit) const
    {
        return encode(buffer, value, idx);
    }

    size_t encode(T* buffer, const V& value, size_t idx) const
    {
        size_t pos = idx;
        pos += size_codec_.encode(buffer, value.size(), pos);

        size_t len = value.size();

        for (size_t c = 0; c < len; c++)
        {
            buffer[pos++] = value[c];
        }

        return pos - idx;
    }

    void move(T* buffer, size_t from, size_t to, size_t size) const
    {
        CopyBuffer(buffer + from, buffer + to, size);
    }

    void copy(const T* src, size_t from, T* tgt, size_t to, size_t size) const
    {
        CopyBuffer(src + from, tgt + to, size);
    }
};
*/


}
