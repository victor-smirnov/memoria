
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/tools/accessors.hpp>

#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer.hpp>
#include <memoria/core/packed/array/packed_vle_array.hpp>
#include <memoria/core/packed/array/packed_fse_array.hpp>

#include <memoria/api/common/packed_api.hpp>

#include <memoria/core/packed/misc/packed_map_so.hpp>

#include <type_traits>

namespace memoria {

template <typename KeyDataType, typename ValueDataType>
class PackedMap: public PackedAllocator {
    using Base = PackedAllocator;
public:
    using MyType = PackedMap;

    static constexpr uint32_t VERSION = 1;

    using KeysPkdStruct = PackedDataTypeBufferT<KeyDataType, true>;
    using ValuesPkdStruct = PackedDataTypeBufferT<ValueDataType, false>;

    using KeyView   = DTTViewType<KeyDataType>;
    using ValueView = DTTViewType<ValueDataType>;

    using SparseObject = PackedMapSO<PackedMap>;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                typename KeysPkdStruct::FieldsList,
                typename ValuesPkdStruct::FieldsList,
                ConstValue<uint32_t, VERSION>
    >;

    enum {KEYS, VALUES, STRUCTS_NUM__};

    using Base::block_size;

    KeysPkdStruct* keys() {
        return this->template get<KeysPkdStruct>(KEYS);
    }

    const KeysPkdStruct* keys() const {
        return this->template get<KeysPkdStruct>(KEYS);
    }


    const ValuesPkdStruct* values() const {
        return this->template get<ValuesPkdStruct>(VALUES);
    }

    ValuesPkdStruct* values() {
        return this->template get<ValuesPkdStruct>(VALUES);
    }



    static int32_t empty_size()
    {
        int32_t parent_size = PackedAllocator::empty_size(STRUCTS_NUM__);
        return parent_size + KeysPkdStruct::empty_size() + ValuesPkdStruct::empty_size();
    }



    VoidResult init() noexcept
    {
        MEMORIA_TRY_VOID(Base::init(empty_size(), STRUCTS_NUM__));

        MEMORIA_TRY_VOID(allocateEmpty<KeysPkdStruct>(KEYS));
        MEMORIA_TRY_VOID(allocateEmpty<ValuesPkdStruct>(VALUES));

        return VoidResult::of();
    }

    int32_t size() const
    {
        return keys()->size();
    }

    void check() const
    {
        keys()->check();
        values()->check();
    }


    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::serialize(buf));

        MEMORIA_TRY_VOID(keys()->serialize(buf));
        return values()->serialize(buf);
    }

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::deserialize(buf));

        MEMORIA_TRY_VOID(keys()->deserialize(buf));
        return values()->deserialize(buf);
    }
};

template <typename KeyDataType, typename ValueDataType>
struct PackedStructTraits<PackedMap<KeyDataType, ValueDataType>> {

};

}
