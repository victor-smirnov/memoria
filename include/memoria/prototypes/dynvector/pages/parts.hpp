
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_PAGES_PARTS_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_PAGES_PARTS_HPP


#include <memoria/core/tools/reflection.hpp>

#include <memoria/prototypes/btree/pages/node_base.hpp>
#include <memoria/prototypes/btree/names.hpp>

#include <memoria/prototypes/dynvector/names.hpp>

namespace memoria    {

using memoria::dynvector::IndexPagePrefixName;

#pragma pack(1)


MEMORIA_PAGE_PART_BEGIN2(RootProfile, IndexPagePrefixName<Indexes>, Int Indexes)

    BigInt prefixes_[Indexes];

    PagePart(): Base() {}

    const BigInt& prefixes(Int idx) const {
        return prefixes_[idx];
    }

    BigInt& prefixes(Int idx) {
        return prefixes_[idx];
    }

    template <template <typename> class FieldFactory>
    void BuildFieldsList(MetadataList &list, Long &abi_ptr) const
    {
        Base::template BuildFieldsList<FieldFactory>(list, abi_ptr);
        FieldFactory<BigInt>::create(list,  prefixes_[0], "PREFIXES", Indexes, abi_ptr);
    }

    template <typename PageType>
    void CopyFrom(PageType* page)
    {
        Base::CopyFrom(page);

        for (Int c = 0; c < Indexes; c++) {
            prefixes(c) = page->prefixes(c);
        }
    }

MEMORIA_PAGE_PART_END


#pragma pack()

}

#endif
