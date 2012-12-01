
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_PARTS_HPP
#define _MEMORIA_PROTOTYPES_BTREE_PAGES_PARTS_HPP


#include <memoria/core/tools/reflection.hpp>
#include <memoria/prototypes/btree/pages/node_base.hpp>
#include <memoria/prototypes/btree/names.hpp>


namespace memoria    {

using namespace memoria::btree;

#pragma pack(1)

template <typename ID>
class BTreeMetadata
{
    static const UInt VERSION = 1;

    static const Int ROOTS = 2;

    BigInt  model_name_;
    BigInt  key_count_;

    Int     branching_factor_;

    ID      roots_[2];

public:

    typedef TypeList<
                ConstValue<UInt, VERSION>,
                ConstValue<UInt, ROOTS>,
                decltype(model_name_),
                decltype(key_count_),
                decltype(branching_factor_),
                ID
    >                                                                           FieldsList;

    BTreeMetadata() {}

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



    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("ROOT_METADATA");

        handler->value("MODEL_NAME",        &model_name_);
        handler->value("KEY_COUNT",         &key_count_);
        handler->value("BRANCHING_FACTOR",  &branching_factor_);

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


MEMORIA_PAGE_PART_BEGIN1(RootNodeMetadataName, TheContainerName)

    static const UInt VERSION = 1;

    typedef TheContainerName       Metadata;

private:
    Metadata metadata_;
public:

    typedef typename MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                typename Metadata::FieldsList
    >::Result                                                                   FieldsList;

    PagePart(): Base(), metadata_() {
        cout<<"RootPage.id="<<Base::id().value()<<endl;
        init();
    }

    void init() {
        Base::init();
        Base::set_root(true);
    }

    const Metadata &metadata() const
    {
        return metadata_;
    }

    Metadata &metadata()
    {
        return metadata_;
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);
        metadata_.generateDataEvents(handler);
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        FieldFactory<Metadata>::serialize(buf, metadata_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        FieldFactory<Metadata>::deserialize(buf, metadata_);
    }

    //This part must not contain copyFrom method because it is
    //contained only in root pages

MEMORIA_PAGE_PART_END


#pragma pack()

}

#endif
