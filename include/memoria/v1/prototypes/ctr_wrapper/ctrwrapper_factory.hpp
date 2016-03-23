
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/ctr_wrapper/container/ctrwrp_c_base.hpp>
#include <memoria/v1/prototypes/ctr_wrapper/iterator/ctrwrp_i_base.hpp>

#include <memoria/v1/prototypes/ctr_wrapper/ctrwrapper_names.hpp>
#include <memoria/v1/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria {

template <typename Profile_, typename ContainerTypeSelector>
struct WrapperTypes {

    typedef ContainerTypeSelector                                               ContainerTypeName;

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
        typedef CtrWrapperCtrBase1<Types_>                  Type;
    };
};



template <typename Profile_, typename T, typename CtrName>
class CtrTF<Profile_, CtrWrapper<CtrName>, T> {

    typedef CtrTF<Profile_, CtrWrapper<CtrName>, T>                             MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

    typedef WrapperTypes<Profile_, CtrWrapper<CtrName>>                         TypesBase;

public:

    struct Types: TypesBase {
        typedef MyType::Allocator                           Allocator;
    };
};

}
