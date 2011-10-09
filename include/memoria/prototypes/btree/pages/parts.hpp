
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
    BigInt model_name_;

public:
    BTreeMetadata() {}

    BigInt &model_name() {
        return model_name_;
    }

    const BigInt &model_name() const {
        return model_name_;
    }

    MetadataList GetFields(Long &abi_ptr) const {
        MetadataList list;
        FieldFactory<BigInt>::create(list, model_name_, "MODEL_NAME", abi_ptr);
        return list;
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

    //This part must not contain CopyFrom method because it is
    //contained only in root pages

MEMORIA_PAGE_PART_END


#pragma pack()

}

#endif
