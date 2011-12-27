
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_PAGES_PARTS_HPP
#define	_MEMORIA_MODELS_ARRAY_PAGES_PARTS_HPP

#include <memoria/prototypes/btree/pages/parts.hpp>

namespace memoria    {


template <typename Allocator>
class ArrayMetadata: public BTreeMetadata<Allocator>
{
    typedef typename Allocator::Page::ID	ID;

	ID roots_[8];

public:
    ArrayMetadata() {}

    MetadataList GetFields(Long &abi_ptr) const
    {
    	MetadataList list = BTreeMetadata<Allocator>::GetFileds(abi_ptr);
    	FieldFactory<BigInt>::create(list, roots_, "ROOTS", 8, abi_ptr);
        return list;
    }

    const ID& roots(Int idx) const {
    	return roots_[idx];
    }

    ID& roots(Int idx) {
    	return roots_[idx];
    }
};


template <typename Profile>
class BTreeRootMetadataTypeFactory<Profile, BTreeRootMetadataFactory<memoria::Vector> > {
    typedef typename ContainerCollectionCfg<Profile>::Types::AllocatorType        Allocator;

public:
    typedef ArrayMetadata<Allocator>                                              Type;
};


}

#endif
