
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

using namespace v1::bt;


template <typename ID>
class BalancedTreeMetadata
{
    static const uint32_t VERSION = 1;

public:
    static const int32_t ROOTS = 2;

private:
    UUID  model_name_;

    int32_t   branching_factor_;

    int32_t   page_size_;

    ID    roots_[ROOTS];

    UUID  txn_id_;

public:

    using FieldsList = TL<
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, ROOTS>,
                decltype(model_name_),
                decltype(branching_factor_),
                decltype(page_size_),
                decltype(txn_id_),
                ID
    >;

    BalancedTreeMetadata()  = default;

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

    int32_t &page_size()
    {
        return page_size_;
    }

    const int32_t &page_size() const
    {
        return page_size_;
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("ROOT_METADATA");

        handler->value("MODEL_NAME",        &model_name_);

        handler->value("BRANCHING_FACTOR",  &branching_factor_);
        handler->value("PAGE_SIZE",         &page_size_);

        handler->startLine("ROOTS", ROOTS);

        for (int32_t c = 0; c < ROOTS; c++)
        {
            handler->value("ROOT",  &roots_[c]);
        }

        handler->endLine();

        handler->value("TXN_ID", &txn_id_);

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        FieldFactory<UUID>::serialize(buf, model_name_);
        FieldFactory<int32_t>::serialize(buf,  branching_factor_);
        FieldFactory<int32_t>::serialize(buf,  page_size_);

        for (int32_t c = 0; c < ROOTS; c++)
        {
            FieldFactory<ID>::serialize(buf, roots_[c]);
        }

        FieldFactory<UUID>::serialize(buf, txn_id_);
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<UUID>::deserialize(buf, model_name_);
        FieldFactory<int32_t>::deserialize(buf,  branching_factor_);
        FieldFactory<int32_t>::deserialize(buf,  page_size_);

        for (int32_t c = 0; c < ROOTS; c++)
        {
            FieldFactory<ID>::deserialize(buf, roots_[c]);
        }

        FieldFactory<UUID>::deserialize(buf, txn_id_);
    }

    const ID& roots(const UUID& idx) const {
        return roots_[idx.lo()];
    }

    ID& roots(const UUID& idx) {
        return roots_[idx.lo()];
    }

    auto& txn_id() {
        return txn_id_;
    }

    const auto& txn_id() const {
        return txn_id_;
    }
};

template <typename ID>
struct TypeHash<BalancedTreeMetadata<ID>>: UInt32Value<
    HashHelper<2500, TypeHash<ID>::Value>::Value
> {};


template <typename ID>
struct FieldFactory<BalancedTreeMetadata<ID>> {

    using Type = BalancedTreeMetadata<ID>;

    static void serialize(SerializationData& data, const Type& field)
    {
        field.serialize(data);
    }

    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            field[c].serialize(data);
        }
    }


    static void deserialize(DeserializationData& data, Type& field)
    {
        field.deserialize(data);
    }

    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            field[c].deserialize(data);
        }
    }
};


}}
