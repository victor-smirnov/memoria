
// Copyright 2021 Victor Smirnov
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

#include <memoria/profiles/common/common.hpp>
#include <memoria/profiles/common/block.hpp>
#include <memoria/profiles/core_api/core_api_profile.hpp>

#include <memoria/core/container/store.hpp>
#include <memoria/core/tools/cow.hpp>
#include <memoria/core/tools/uuid.hpp>

namespace memoria {

template <typename ValueHolder>
struct FieldFactory<CowBlockID<ValueHolder>> {
    using Type = CowBlockID<ValueHolder>;

    static constexpr size_t Size = sizeof(ValueHolder);

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        std::memcpy(data.buf, &field.value(), Size);
        data.buf    += Size;
        data.total  += Size;
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        std::memcpy(&field.value(), data.buf, Size);
        data.buf += Size;
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            std::memcpy(data.buf, &field[c].value(), Size);
            data.buf    += Size;
            data.total  += Size;
        }
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            std::memcpy(&field[c].value(), data.buf, Size);
            data.buf += Size;
        }
    }
};

}
