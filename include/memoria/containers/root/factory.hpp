
// Copyright Victor Smirnov, Ivan Yurchenko 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ROOT_FACTORY_HPP
#define _MEMORIA_MODELS_ROOT_FACTORY_HPP

#include <memoria/containers/map/factory.hpp>

#include <memoria/containers/root/container/root_c_api.hpp>

#include <memoria/containers/root/pages/parts.hpp>

namespace memoria {

template <typename Profile>
struct BTreeTypes<Profile, memoria::Root>: public BTreeTypes<Profile, memoria::Map<BigInt, NullType, 1> > {

    typedef BTreeTypes<Profile, memoria::Map<BigInt, NullType, 1>>              Base;

    typedef typename Base::ID                                                   Value;

    typedef typename MergeLists<
                	typename Base::ContainerPartsList,
                    memoria::root::CtrApiName
    >::Result                                                                   ContainerPartsList;

    typedef RootCtrMetadata<typename Base::ID>                                  Metadata;
};



template <typename Profile, typename T>
class CtrTF<Profile, memoria::Root, T>: public CtrTF<Profile, memoria::Map<BigInt, NullType, 1>, T> {
};

}

#endif
