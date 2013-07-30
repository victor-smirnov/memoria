
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_PARTS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_PARTS_HPP

#include <memoria/prototypes/balanced_tree/bt_types.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/types/typehash.hpp>


namespace memoria    {

using namespace memoria::balanced_tree;



// FIXME: Make one more level of root metadata hierarchy and
// move model_name and page_size fields into it.

template <typename ID, Int Streams>
class BalancedTreeMetadata
{
    static const UInt VERSION = 1;

    static const Int ROOTS = 2;

    BigInt  model_name_;
    BigInt  size_[Streams];

    Int     branching_factor_;

    Int     page_size_;

    ID      roots_[2];

public:

    typedef TypeList<
                ConstValue<UInt, VERSION>,
                ConstValue<UInt, ROOTS>,
                decltype(model_name_),
                decltype(size_[0]),
                ConstValue<UInt, Streams>,
                decltype(branching_factor_),
                decltype(page_size_),
                ID
    >                                                                           FieldsList;

    BalancedTreeMetadata() {}

    BigInt &model_name()
    {
        return model_name_;
    }

    const BigInt &model_name() const
    {
        return model_name_;
    }

    BigInt &size(Int idx)
    {
        return size_[idx];
    }

    const BigInt &size(Int idx) const
    {
        return size_[idx];
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

        handler->startGroup("STREAM_SIZES", Streams);

        for (Int c = 0; c < Streams; c++)
        {
        	handler->value("SIZE",  size_ + c);
        }

        handler->endGroup();

        handler->value("BRANCHING_FACTOR",  &branching_factor_);
        handler->value("PAGE_SIZE",  		&page_size_);

        handler->startGroup("ROOTS", ROOTS);

        for (Int c = 0; c < ROOTS; c++)
        {
            IDValue id(roots_[c]);
            handler->value("ROOT",  &id);
        }

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        FieldFactory<BigInt>::serialize(buf, model_name_);
        FieldFactory<BigInt>::serialize(buf, size_, Streams);
        FieldFactory<Int>::serialize(buf,    branching_factor_);
        FieldFactory<Int>::serialize(buf,    page_size_);

        for (Int c = 0; c < ROOTS; c++)
        {
            FieldFactory<ID>::serialize(buf, roots(c));
        }
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<BigInt>::deserialize(buf, model_name_);
        FieldFactory<BigInt>::deserialize(buf, size_, Streams);
        FieldFactory<Int>::deserialize(buf,    branching_factor_);
        FieldFactory<Int>::deserialize(buf,    page_size_);

        for (Int c = 0; c < ROOTS; c++)
        {
            FieldFactory<ID>::deserialize(buf, roots(c));
        }
    }

    const ID& roots(Int idx) const {
        return roots_[idx];
    }

    ID& roots(Int idx) {
        return roots_[idx];
    }
};

template <typename ID, Int Streams>
struct TypeHash<BalancedTreeMetadata<ID, Streams>>: UIntValue<
	HashHelper<2500, TypeHash<ID>::Value, Streams>::Value
> {};

}

#endif
