
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

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/profiles/common/common.hpp>

#include <memoria/core/packed/misc/packed_so_default.hpp>

namespace memoria {

//template <typename BalancedTreeMetadata>
//class BalancedTreeMetadataSO {
//    BalancedTreeMetadata* data_;
//public:
//    BalancedTreeMetadataSO(BalancedTreeMetadata* data): data_(data) {}

//    BalancedTreeMetadata* operator->() {return data_;}
//    const BalancedTreeMetadata* operator->() const {return data_;}

//    void generateDataEvents(IBlockDataEventHandler* handler) const
//    {
//        data_->generateDataEvents(handler);
//    }
//};



template <typename Profile>
class BalancedTreeMetadata
{
    static const uint32_t VERSION = 1;

public:
    using CtrID = ProfileCtrID<Profile>;

    using SparseObject = PackedDefaultSO<BalancedTreeMetadata>;

private:
    PackedAllocatable header_;

    CtrID  model_name_;

    int32_t block_size_;

public:

    using FieldsList = TL<
                ConstValue<uint32_t, VERSION>,
                decltype(model_name_),
                decltype(block_size_)
    >;

    BalancedTreeMetadata() = default;

    CtrID &model_name()
    {
        return model_name_;
    }

    const CtrID &model_name() const
    {
        return model_name_;
    }

    int32_t &memory_block_size()
    {
        return block_size_;
    }

    const int32_t &memory_block_size() const
    {
        return block_size_;
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startGroup("ROOT_METADATA");

        handler->value("CTR_ID", &model_name_);

        handler->value("BLOCK_SIZE", &block_size_);

        handler->endGroup();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<CtrID>::serialize(buf, model_name_);
        FieldFactory<int32_t>::serialize(buf,  block_size_);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<CtrID>::deserialize(buf, model_name_);
        FieldFactory<int32_t>::deserialize(buf,  block_size_);
    }

    static int32_t empty_size() {
        return sizeof(BalancedTreeMetadata);
    }

    OpStatus init() {
        return OpStatus::OK;
    }

    OpStatus pack() {
        return OpStatus::OK;
    }
};

template <typename Profile>
struct TypeHash<BalancedTreeMetadata<Profile>>: UInt64Value<
    HashHelper<
        2500, 
        TypeHash<typename BalancedTreeMetadata<Profile>::CtrID>::Value & 0xFFFFFFFF,
        (TypeHash<typename BalancedTreeMetadata<Profile>::CtrID>::Value >> 32) & 0xFFFFFFFF
    >
> {};


template <typename Profile>
struct FieldFactory<BalancedTreeMetadata<Profile>> {

    using Type = BalancedTreeMetadata<Profile>;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        field.serialize(data);
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            field[c].serialize(data);
        }
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        field.deserialize(data);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            field[c].deserialize(data);
        }
    }
};


}
