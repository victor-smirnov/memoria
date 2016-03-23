
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#pragma once

#include <memoria/v1/prototypes/composite/comp_names.hpp>
#include <memoria/v1/core/container/container.hpp>

namespace memoria    {



template <typename Profile_, typename ContainerTypeSelector>
struct CompositeTypes {

    typedef Profile_                                                            Profile;

    typedef TypeList<
        memoria::bt::AllocatorName
    >                                                                           ContainerPartsList;

    typedef TypeList<>                                                          IteratorPartsList;

    typedef EmptyType                                                           IteratorInterface;

    template <typename Types_>
    struct IterBaseFactory {
        typedef IteratorBase<Types_>                        Type;
    };

    template <typename Types_>
    struct CtrBaseFactory {
        typedef CtrBase<Types_>                             Type;
    };
};



template <typename Profile_, typename T>
class CtrTF<Profile_, memoria::Composite, T> {

    typedef CtrTF<Profile_, Composite, T>                                       MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

public:

    struct Types {
        typedef Profile_                                    Profile;
        typedef MyType::Allocator                           Allocator;

        typedef CtrTypesT<Types>                            CtrTypes;
        typedef IterTypesT<Types>                           IterTypes;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;


};


}
