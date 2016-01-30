
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_FACTORY_HPP
#define _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_FACTORY_HPP

#include <memoria/containers/root/root_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>
#include <memoria/containers/multimap/mmap_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/table/table_factory.hpp>


#include <memoria/core/container/metadata_repository.hpp>

#include "../inmem/names.hpp"
#include "allocator.hpp"

namespace memoria {

//template <typename Profile>
//class ContainerCollectionCfg;
//
//template <typename T>
//class ContainerCollectionCfg<DefaultProfile<T> > {
//public:
//	using Types = BasicContainerCollectionCfg<DefaultProfile<T>>;
//};
//
//
//using PersistentInMemAllocator = memoria::PersistentInMemAllocatorT<
//	DefaultProfile<>,
//	ContainerCollectionCfg<DefaultProfile<>>::Types::Page
//>;
//

}

#endif
