
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

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/tools/accessors.hpp>

#include <memoria/v1/core/packed/array/packed_vle_array.hpp>
#include <memoria/v1/core/packed/array/packed_fse_array.hpp>

#include <memoria/v1/api/common/packed_api.hpp>

#include <memoria/v1/core/packed/misc/packed_map_so.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

namespace _ {

    template <typename DataType, bool FixedSize = DataTypeTraits<DataType>::isFixedSize>
    struct KeysAdapter;

    template <typename DataType>
    struct KeysAdapter<DataType, false> {
        using PkdStruct = PackedVLenElementArray<PackedVLenElementArrayTypes<DataType, 1, 1>>;
    };

    template <typename DataType>
    struct KeysAdapter<DataType, true> {
        using PkdStruct = PackedFixedSizeElementArray<PackedFixedSizeElementArrayTypes<DataType, 1, 1>>;
    };


    template <typename DataType, bool FixedSize = DataTypeTraits<DataType>::isFixedSize>
    struct ValuesAdapter;

    template <typename DataType>
    struct ValuesAdapter<DataType, false> {
        using PkdStruct = PackedVLenElementArray<PackedVLenElementArrayTypes<DataType, 1, 1>>;
    };

    template <typename DataType>
    struct ValuesAdapter<DataType, true> {
        using PkdStruct = PackedFixedSizeElementArray<PackedFixedSizeElementArrayTypes<DataType, 1, 1>>;
    };

}



template <typename KeyDataType, typename ValueDataType>
class PackedMap: public PackedAllocator {
    using Base = PackedAllocator;
public:
    using MyType = PackedMap;

    static constexpr uint32_t VERSION = 1;

    using KeysPkdStruct      = typename _::KeysAdapter<KeyDataType>::PkdStruct;
    using ValuesPkdStruct    = typename _::ValuesAdapter<ValueDataType>::PkdStruct;

    using KeyView   = typename DataTypeTraits<KeyDataType>::ViewType;
    using ValueView = typename DataTypeTraits<ValueDataType>::ViewType;

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



    OpStatus init()
    {
        if(isFail(Base::init(empty_size(), STRUCTS_NUM__))) {
            return OpStatus::FAIL;
        }

        KeysPkdStruct* keys = allocateEmpty<KeysPkdStruct>(KEYS);
        if(isFail(keys)) {
            return OpStatus::FAIL;
        }


        ValuesPkdStruct* values = allocateEmpty<ValuesPkdStruct>(VALUES);
        if(isFail(values)) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
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
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        keys()->serialize(buf);
        values()->serialize(buf);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        keys()->deserialize(buf);
        values()->deserialize(buf);
    }
};

template <typename KeyDataType, typename ValueDataType>
struct PackedStructTraits<PackedMap<KeyDataType, ValueDataType>> {

};





}}
