
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_PARTS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_PARTS_HPP

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/uuid.hpp>


namespace memoria    {

using namespace memoria::bt;


template <typename ID>
class BalancedTreeMetadata
{
    static const UInt VERSION = 1;

    static const Int ROOTS = 2;

    UUID  model_name_;

    Int   branching_factor_;

    Int   page_size_;

    ID    roots_[ROOTS];

    UUID  txn_id_;

public:

    using FieldsList = TL<
                ConstValue<UInt, VERSION>,
                ConstValue<UInt, ROOTS>,
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


    Int &branching_factor()
    {
        return branching_factor_;
    }

    const Int &branching_factor() const
    {
        return branching_factor_;
    }

    Int &page_size()
    {
        return page_size_;
    }

    const Int &page_size() const
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

        for (Int c = 0; c < ROOTS; c++)
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
        FieldFactory<Int>::serialize(buf,  branching_factor_);
        FieldFactory<Int>::serialize(buf,  page_size_);

        for (Int c = 0; c < ROOTS; c++)
        {
            FieldFactory<ID>::serialize(buf, roots_[c]);
        }

        FieldFactory<UUID>::serialize(buf, txn_id_);
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<UUID>::deserialize(buf, model_name_);
        FieldFactory<Int>::deserialize(buf,  branching_factor_);
        FieldFactory<Int>::deserialize(buf,  page_size_);

        for (Int c = 0; c < ROOTS; c++)
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
struct TypeHash<BalancedTreeMetadata<ID>>: UIntValue<
    HashHelper<2500, TypeHash<ID>::Value>::Value
> {};


template <typename ID>
struct FieldFactory<BalancedTreeMetadata<ID>> {

    using Type = BalancedTreeMetadata<ID>;

    static void serialize(SerializationData& data, const Type& field)
    {
        field.serialize(data);
    }

    static void serialize(SerializationData& data, const Type* field, Int size)
    {
        for (Int c = 0; c < size; c++)
        {
            field[c].serialize(data);
        }
    }


    static void deserialize(DeserializationData& data, Type& field)
    {
        field.deserialize(data);
    }

    static void deserialize(DeserializationData& data, Type* field, Int size)
    {
        for (Int c = 0; c < size; c++)
        {
            field[c].deserialize(data);
        }
    }
};


}

#endif
