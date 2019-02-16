
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

#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/core/tools/reflection.hpp>
#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/core/tools/uuid.hpp>


namespace memoria {
namespace v1 {




template <typename ID>
class BalancedTreeMetadata
{
    static const uint32_t VERSION = 1;

public:
    static const int32_t ROOTS = 2;
    static const int32_t LINKS = 2;
    static const int32_t DESCRIPTOR_SIZE = 200;

private:
    UUID  model_name_;

    int32_t   branching_factor_;

    int32_t   block_size_;

    ID    roots_[ROOTS];
    ID    links_[LINKS];

    UUID  type_tag_;


    UUID  txn_id_;

    uint8_t descriptor_[DESCRIPTOR_SIZE];

public:

    using FieldsList = TL<
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, ROOTS>,
                decltype(model_name_),
                decltype(branching_factor_),
                decltype(block_size_),
                decltype(txn_id_),
                ID,
                ID,
                decltype(type_tag_),
                ConstValue<uint32_t, DESCRIPTOR_SIZE>
    >;

    BalancedTreeMetadata() = default;

    UUID &model_name()
    {
        return model_name_;
    }

    const UUID &model_name() const
    {
        return model_name_;
    }


    int32_t &branching_factor()
    {
        return branching_factor_;
    }

    const int32_t &branching_factor() const
    {
        return branching_factor_;
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

        handler->value("MODEL_NAME",        &model_name_);

        handler->value("BRANCHING_FACTOR",  &branching_factor_);
        handler->value("PAGE_SIZE",         &block_size_);

        handler->startLine("ROOTS", ROOTS);

        for (int32_t c = 0; c < ROOTS; c++)
        {
            handler->value("ROOT",  &roots_[c]);
        }

        handler->endLine();

        handler->startLine("LINKS", LINKS);
        for (int32_t c = 0; c < LINKS; c++)
        {
            handler->value("LINK",  &links_[c]);
        }
        handler->endLine();

        handler->value("TYPE_TAG", &type_tag_);
        handler->value("TXN_ID", &txn_id_);

        handler->value("DESCRIPTOR", descriptor_, DESCRIPTOR_SIZE, IBlockDataEventHandler::BYTE_ARRAY);

        handler->endGroup();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<UUID>::serialize(buf, model_name_);
        FieldFactory<int32_t>::serialize(buf,  branching_factor_);
        FieldFactory<int32_t>::serialize(buf,  block_size_);

        for (int32_t c = 0; c < ROOTS; c++)
        {
            FieldFactory<ID>::serialize(buf, roots_[c]);
        }

        for (int32_t c = 0; c < LINKS; c++)
        {
            FieldFactory<ID>::serialize(buf, links_[c]);
        }

        FieldFactory<UUID>::serialize(buf, type_tag_);
        FieldFactory<ID>::serialize(buf, txn_id_);
        FieldFactory<uint8_t>::serialize(buf, descriptor_, DESCRIPTOR_SIZE);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<UUID>::deserialize(buf, model_name_);
        FieldFactory<int32_t>::deserialize(buf,  branching_factor_);
        FieldFactory<int32_t>::deserialize(buf,  block_size_);

        for (int32_t c = 0; c < ROOTS; c++)
        {
            FieldFactory<ID>::deserialize(buf, roots_[c]);
        }

        for (int32_t c = 0; c < LINKS; c++)
        {
            FieldFactory<ID>::deserialize(buf, links_[c]);
        }

        FieldFactory<ID>::deserialize(buf, type_tag_);
        FieldFactory<ID>::deserialize(buf, txn_id_);
        FieldFactory<uint8_t>::deserialize(buf, descriptor_, DESCRIPTOR_SIZE);
    }

    const ID& roots(const UUID& idx) const {
        return roots_[idx.lo()];
    }

    ID& roots(const UUID& idx) {
        return roots_[idx.lo()];
    }

    const ID& links(int32_t idx) const
    {
        if (idx >= 0 && idx < LINKS) {
            return links_[idx];
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Link num is out of range");
        }
    }

    ID& links(int32_t idx)
    {
        if (idx >= 0 && idx < LINKS) {
            return links_[idx];
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Link num is out of range");
        }
    }

    auto& txn_id() {
        return txn_id_;
    }

    const auto& txn_id() const {
        return txn_id_;
    }

    const UUID& type_tag() const {
        return type_tag_;
    }

    UUID& type_tag() {
        return type_tag_;
    }


    int32_t roots_num() const {
        return ROOTS;
    }

    int32_t links_num() const {
        return LINKS;
    }

    uint8_t* descriptor() {return descriptor_;}
    const uint8_t* descriptor() const {return descriptor_;}

    std::string descriptor_str() const {
        return std::string(T2T<const char*>(descriptor()));
    }

    void set_descriptor(const std::string& str)
    {
        std::memset(descriptor(), 0, DESCRIPTOR_SIZE);
        size_t to_copy = (str.length() < DESCRIPTOR_SIZE) ? str.length() : DESCRIPTOR_SIZE - 1;
        std::memcpy(descriptor(), str.data(), to_copy);
    }

    int32_t descriptor_size() const {
        return DESCRIPTOR_SIZE - 1;
    }
};

template <typename ID>
struct TypeHash<BalancedTreeMetadata<ID>>: UInt64Value<
    HashHelper<
        2500, 
        TypeHash<ID>::Value & 0xFFFFFFFF, 
        (TypeHash<ID>::Value >> 32) & 0xFFFFFFFF 
    >
> {};


template <typename ID>
struct FieldFactory<BalancedTreeMetadata<ID>> {

    using Type = BalancedTreeMetadata<ID>;

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


}}
