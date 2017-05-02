
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

#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <ostream>

namespace memoria {
namespace v1 {

template <int32_t Size>
class FixedArray {
    using MyType = FixedArray<Size>;
    using T = uint8_t;

    T data_[Size];
public:
    FixedArray() {}

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

    T& operator[](size_t c) {
        return data_[c];
    }

    const T& operator[](size_t c) const {
        return data_[c];
    }
};

template <int32_t Size>
std::ostream& operator<<(std::ostream& out, const FixedArray<Size>& array)
{
    std::ios state(nullptr);
    state.copyfmt(out);

    out<<std::setbase(16);
    for (int32_t c = 0; c < Size; c++)
    {
        out<<std::setw(2)<<setfill('0');
        out << (uint32_t)array[c];
    }

    out.copyfmt(state);

    return out;
}




template <typename T> struct FieldFactory;


template <int32_t Size>
struct FieldFactory<FixedArray<Size> > {
    using Type = FixedArray<Size>;

    static void serialize(SerializationData& data, const Type& field)
    {
        memmove(data.buf, field.data(), Size);
        data.buf    += Size;
        data.total  += Size;
    }

    static void deserialize(DeserializationData& data, Type& field)
    {
        memmove(field.data(), data.buf, Size);
        data.buf += Size;
    }

    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            memmove(data.buf, field[c].data(), Size);
            data.buf    += Size;
            data.total  += Size;
        }

    }

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
    static const uint32_t Value = HashHelper<23423, 68751234, 1524857, Size>::Value;
};


template <int32_t Size>
struct IOBufferAdapter<FixedArray<Size>> {

    template <typename IOBuffer>
    static bool put(IOBuffer& buffer, const FixedArray<Size>& value)
    {
        buffer.putVLen(Size);
        return buffer.put(value.data(), Size);
    }

    template <typename IOBuffer>
    static FixedArray<Size> get(IOBuffer& buffer)
    {
        int64_t len = buffer.getVLen();
        if (len <= Size)
        {
            FixedArray<Size> array;

            buffer.get(array.data(), len);

            for (int64_t c = len; c < Size; c++) {
                array[c] = 0;
            }

            return array;
        }
        else {
            throw Exception(MA_RAW_SRC, SBuf() << "Array length " << len << " exceeds fixed limit of " << Size);
        }
    }
};



}}
