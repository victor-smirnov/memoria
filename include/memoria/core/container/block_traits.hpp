
// Copyright 2011 Victor Smirnov
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

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/profiles/common/block.hpp>


namespace memoria {

template <typename T> struct FieldFactory;

template <typename T>
struct FieldFactory<BlockID<T> > {
private:
    using Type = BlockID<T>;

public:
    static void serialize(SerializationData& data, const Type& field, int32_t count = 1)
    {
        memmove(data.buf, &field, sizeof(Type) * count);

        data.buf    += sizeof(Type) * count;
        data.total  += sizeof(Type) * count;
    }

    static void serialize(SerializationData& data, const Type* field, int32_t count = 1)
    {
        memmove(data.buf, field, sizeof(Type) * count);

        data.buf    += sizeof(Type) * count;
        data.total  += sizeof(Type) * count;
    }

    static void deserialize(DeserializationData& data, Type& field, int32_t count = 1)
    {
        memmove(&field, data.buf, sizeof(Type) * count);
        data.buf += sizeof(Type) * count;
    }

    static void deserialize(DeserializationData& data, Type* field, int32_t count = 1)
    {
        memmove(field, data.buf, sizeof(Type) * count);
        data.buf += sizeof(Type) * count;
    }
};


template <int32_t Size>
struct FieldFactory<BitBuffer<Size> > {
    using Type = BitBuffer<Size>;

    static void serialize(SerializationData& data, const Type& field)
    {
        memmove(data.buf, &field, sizeof(Type));
        data.buf    += sizeof(Type);
        data.total  += sizeof(Type);
    }

    static void deserialize(DeserializationData& data, Type& field)
    {
        memmove(&field, data.buf, sizeof(Type));
        data.buf += sizeof(Type);
    }
};

}
