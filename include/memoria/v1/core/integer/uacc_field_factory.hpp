
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/type2type.hpp>

#include <memoria/v1/core/integer/integer.hpp>

#include <ostream>

namespace memoria {
namespace v1 {




template <typename T> struct FieldFactory;


template <size_t BitLength>
struct FieldFactory<UnsignedAccumulator<BitLength>> {
    using Type = UnsignedAccumulator<BitLength>;

    static constexpr size_t Size = Type::Size * sizeof(typename Type::ValueT);

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        std::memcpy(data.buf, field.value_, Size);
        data.buf    += Size;
        data.total  += Size;
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        std::memcpy(field.value_, data.buf, Size);
        data.buf += Size;
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            std::memcpy(data.buf, field[c].value_, Size);
            data.buf    += Size;
            data.total  += Size;
        }
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            std::memcpy(field[c].value_, data.buf, Size);
            data.buf += Size;
        }
    }
};




template <typename T> struct TypeHash;

template <size_t BitLength>
struct TypeHash<UnsignedAccumulator<BitLength>> {
    static const uint64_t Value = HashHelper<8533144, 1687234, 9911527, BitLength>;
};


}}
