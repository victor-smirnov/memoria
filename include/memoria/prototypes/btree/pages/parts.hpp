
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_PARTS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_PAGES_PARTS_HPP


#include <memoria/core/tools/reflection.hpp>
#include <memoria/prototypes/btree/pages/node_base.hpp>
#include <memoria/prototypes/btree/names.hpp>


namespace memoria    {

using namespace memoria::btree;

#pragma pack(1)

template <typename Allocator>
class BTreeMetadata
{
	static const Int ROOTS = 2;

	typedef typename Allocator::ID	ID;

    BigInt model_name_;
    BigInt key_count_;

    ID roots_[2];

public:
    BTreeMetadata() {}

    BigInt &model_name() {
        return model_name_;
    }

    const BigInt &model_name() const {
        return model_name_;
    }

    BigInt &key_count() {
        return key_count_;
    }

    const BigInt &key_count() const {
        return key_count_;
    }


    MetadataList GetFields(Long &abi_ptr) const
    {
        MetadataList list;
        FieldFactory<BigInt>::create(list, model_name_, "MODEL_NAME", abi_ptr);
        FieldFactory<BigInt>::create(list, key_count_,  "KEY_COUNT",  abi_ptr);

        MetadataList rootsList;
        for (Int c = 0; c < ROOTS; c++)
        {
        	FieldFactory<ID>::create(rootsList, roots(c), "ROOT", abi_ptr);
        }
        list.push_back(new MetadataGroupImpl("ROOTS", rootsList));

        return list;
    }

    template <template <typename> class FieldFactory>
    void Serialize(SerializationData& buf) const
    {
    	FieldFactory<BigInt>::serialize(buf, model_name_);
    	FieldFactory<BigInt>::serialize(buf, key_count_);

    	for (Int c = 0; c < ROOTS; c++)
    	{
    		FieldFactory<ID>::serialize(buf, roots(c));
    	}
    }

    template <template <typename> class FieldFactory>
    void Deserialize(DeserializationData& buf)
    {
    	FieldFactory<BigInt>::deserialize(buf, model_name_);
    	FieldFactory<BigInt>::deserialize(buf, key_count_);

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


template <typename Profile, typename TheContainerName>
class BTreeRootMetadataTypeFactory<Profile, BTreeRootMetadataFactory<TheContainerName> > {
    typedef typename ContainerCollectionCfg<Profile>::Types::AllocatorType        Allocator;

public:
    typedef BTreeMetadata<Allocator>                                              Type;
};



MEMORIA_PAGE_PART_BEGIN1(RootNodeMetadataName, TheContainerName)

  	typedef TheContainerName       Metadata;

private:
    Metadata metadata_;
public:

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

    template <template <typename> class FieldFactory>
    void BuildFieldsList(MetadataList &list, Long &abi_ptr) const
    {
        Base::template BuildFieldsList<FieldFactory>(list, abi_ptr);
        FieldFactory<Metadata>::create(list,  metadata(), "ROOT_METADATA", abi_ptr);
    }

    template <template <typename> class FieldFactory>
    void Serialize(SerializationData& buf) const
    {
    	Base::template Serialize<FieldFactory>(buf);

    	FieldFactory<Metadata>::serialize(buf, metadata_);
    }

    template <template <typename> class FieldFactory>
    void Deserialize(DeserializationData& buf)
    {
    	Base::template Deserialize<FieldFactory>(buf);

    	FieldFactory<Metadata>::deserialize(buf, metadata_);
    }

    //This part must not contain CopyFrom method because it is
    //contained only in root pages

MEMORIA_PAGE_PART_END


#pragma pack()

}

#endif
