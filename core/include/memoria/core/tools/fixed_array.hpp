
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


#include <memoria/core/reflection/typehash.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/strings/strings.hpp>

#include <memoria/core/datatypes/traits.hpp>

#include <memoria/core/tools/span.hpp>

#include <ostream>

namespace memoria {

template <int32_t Size>
class FixedArray {
    using MyType = FixedArray<Size>;
    using T = uint8_t;

    T data_[Size];
public:
    constexpr FixedArray() = default;

    T* data() {
        return data_;
    }

    static constexpr int32_t size() {
        return Size;
    }

    static constexpr int32_t length() {
        return Size;
    }

    const T* data() const {
        return data_;
    }

    bool operator!=(const MyType& other) const {
        return !operator==(other);
    }

    operator Span<uint8_t> () {
        return Span<uint8_t>(data_, Size);
    }

    operator Span<const uint8_t> () const {
        return Span<const uint8_t>(data_, Size);
    }

    bool operator==(const MyType& other) const
    {
        for (int32_t c = 0; c < Size; c++)
        {
            if (data_[c] != other.data_[c])
            {
                return false;
            }
        }

        return true;
    }

    bool operator<(const MyType& other) const
    {
        return std::lexicographical_compare(data_, data_ + Size, other.data_, other.data_ + Size);
    }

    bool operator<=(const MyType& other) const
    {
        return operator<(other) || operator==(other);
    }

    bool operator>(const MyType& other) const
    {
        return !operator<=(other);
    }

    bool operator>=(const MyType& other) const
    {
        return !operator<(other);
    }

    void swap(MyType& other)
    {
        for (int32_t c = 0; c < Size; c++)
        {
            std::swap(data_[c], other.data_[c]);
        }
    }

    constexpr T& operator[](size_t c) {
        return data_[c];
    }

    constexpr const T& operator[](size_t c) const {
        return data_[c];
    }
};

template <int32_t Size>
std::ostream& operator<<(std::ostream& out, const FixedArray<Size>& array)
{
    std::ios state(nullptr);
    state.copyfmt(out);

    out << std::setbase(16);
    for (int32_t c = 0; c < Size; c++)
    {
        out << std::setw(2) << std::setfill('0');
        out << (uint32_t)array[c];
    }

    out.copyfmt(state);

    return out;
}




template <typename T> struct FieldFactory;


template <int32_t Size>
struct FieldFactory<FixedArray<Size> > {
    using Type = FixedArray<Size>;


    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        memmove(data.buf, field.data(), Size);
        data.buf    += Size;
        data.total  += Size;
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        memmove(field.data(), data.buf, Size);
        data.buf += Size;
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            memmove(data.buf, field[c].data(), Size);
            data.buf    += Size;
            data.total  += Size;
        }

    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            memmove(field[c].data(), data.buf, Size);
            data.buf += Size;
        }
    }
};




template <typename T> struct TypeHash;

template <int32_t Size>
struct TypeHash<FixedArray<Size>> {
    static const uint64_t Value = HashHelper<23423, 68751234, 1524857, Size>;
};


template <int32_t Size>
struct DataTypeTraits<FixedArray<Size>>:  FixedSizeDataTypeTraits<FixedArray<Size>, FixedArray<Size>>
{
    using ViewType   = FixedArray<Size>;

    static constexpr bool HasTypeConstructors = false;

    using DatumSelector = FixedSizeDataTypeTag;
};

}

namespace fmt {

template <int32_t Size>
struct formatter<memoria::FixedArray<Size>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::FixedArray<Size>& d, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", memoria::toString(d));
    }
};

}

