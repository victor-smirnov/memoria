
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_FACTORY_HPP
#define _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_FACTORY_HPP

#include <memoria/containers/map/map_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include "allocator.hpp"

namespace memoria {

template <typename Profile>
class ContainerCollectionCfg;

template <typename T>
class ContainerCollectionCfg<DefaultProfile<T> > {
public:
	using Types = BasicContainerCollectionCfg<DefaultProfile<T>>;
};


//using SmallInMemAllocator = memoria::InMemAllocator<
//	DefaultProfile<>,
//	ContainerCollectionCfg<DefaultProfile<> >::Types::Page
//>;


template <typename CtrName>
using DCtrTF = CtrTF<DefaultProfile<>, CtrName>;

template <typename CtrName>
using DCtr = typename CtrTF<DefaultProfile<>, CtrName>::Type;


}

#endif
