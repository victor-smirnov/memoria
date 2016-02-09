
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_ALLOCATORS_INMEM_FACTORY_HPP
#define _MEMORIA_ALLOCATORS_INMEM_FACTORY_HPP

//#include <memoria/containers/root/root_factory.hpp>
//#include <memoria/containers/map/map_factory.hpp>
//#include <memoria/containers/vector/vctr_factory.hpp>
//#include <memoria/containers/multimap/mmap_factory.hpp>
//#include <memoria/containers/vector_map/vmap_factory.hpp>
//#include <memoria/containers/seq_dense/seqd_factory.hpp>
//#include <memoria/containers/table/table_factory.hpp>
//#include <memoria/containers/labeled_tree/ltree_factory.hpp>
//#include <memoria/containers/wt/wt_factory.hpp>
//#include <memoria/containers/vector_tree/vtree_factory.hpp>
//#include <memoria/containers/dbl_map/dblmap_factory.hpp>
//#include <memoria/containers/smrk_map/smrkmap_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include "names.hpp"
//#include "allocator.hpp"

namespace memoria    {


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
