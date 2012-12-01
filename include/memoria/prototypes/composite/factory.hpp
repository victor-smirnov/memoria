
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_COMPOSITE_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_COMPOSITE_FACTORY_HPP

#include <memoria/prototypes/composite/names.hpp>
#include <memoria/core/container/container.hpp>

namespace memoria    {



template <typename Profile_, typename ContainerTypeSelector>
struct CompositeTypes {

    typedef Profile_                                                        Profile;

    typedef TypeList<
        memoria::btree::AllocatorName
    >                                                                       ContainerPartsList;

    typedef TypeList<>                                                          IteratorPartsList;

    typedef EmptyType                                                       IteratorInterface;

    template <typename Types_>
    struct IterBaseFactory {
        typedef IteratorBase<Types_>                                        Type;
    };

    template <typename Types_>
    struct CtrBaseFactory {
        typedef ContainerBase<Types_>                                       Type;
    };
};



template <typename Profile_, typename T>
class CtrTF<Profile_, memoria::CompositeCtr, T> {

    typedef CtrTF<Profile_, CompositeCtr, T>                                    MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

public:

    struct Types {
        typedef Profile_                        Profile;
        typedef MyType::Allocator               Allocator;

        typedef VectorMapCtrTypes<Types>        CtrTypes;
        typedef VectorMapIterTypes<Types>       IterTypes;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;


};


}

#endif
