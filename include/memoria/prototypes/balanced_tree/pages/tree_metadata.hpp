
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_PARTS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_PARTS_HPP

#include <memoria/prototypes/balanced_tree/baltree_types.hpp>
#include <memoria/core/tools/reflection.hpp>


namespace memoria    {

using namespace memoria::balanced_tree;



// FIXME: Make one more level of root metadata hierarchy and
// move model_name and page_size fields into it.

template <typename ID>
class BalancedTreeMetadata
{
    static const UInt VERSION = 1;

    static const Int ROOTS = 2;

    BigInt  model_name_;
    BigInt  key_count_;

    Int     branching_factor_;

    Int     page_size_;

    ID      roots_[2];

public:

    typedef TypeList<
                ConstValue<UInt, VERSION>,
                ConstValue<UInt, ROOTS>,
                decltype(model_name_),
                decltype(key_count_),
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

    BigInt &key_count()
    {
        return key_count_;
    }

    const BigInt &key_count() const
    {
        return key_count_;
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
        handler->value("KEY_COUNT",         &key_count_);
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
        FieldFactory<BigInt>::serialize(buf, key_count_);
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
        FieldFactory<BigInt>::deserialize(buf, key_count_);
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



}

#endif
