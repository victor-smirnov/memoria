
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_STREAM_FACTORY_HPP
#define _MEMORIA_CONTAINERS_STREAM_FACTORY_HPP

#include <memoria/containers/root/factory.hpp>
#include <memoria/containers/map/factory.hpp>
#include <memoria/containers/set/factory.hpp>
#include <memoria/containers/vector/factory.hpp>
#include <memoria/containers/vector_map/factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include "names.hpp"
#include "allocator.hpp"

namespace memoria    {

using namespace memoria::btree;


template <typename Profile>
class ContainerCollectionCfg;

template <typename T>
class ContainerCollectionCfg<SmallProfile<T> > {
public:
    typedef BasicContainerCollectionCfg<SmallProfile<T> >                               Types;
};


typedef memoria::InMemAllocator<
            SmallProfile<>,
            ContainerCollectionCfg<SmallProfile<> >::Types::Page,
            EmptyType
        >                                                                               SmallInMemAllocator;

typedef CtrTypeFactory<SmallProfile<> > SmallCtrTypeFactory;
MEMORIA_TEMPLATE_EXTERN template class MetadataRepository<SmallProfile<> >;


MEMORIA_EXTERN_BASIC_CONTAINER(SmallCtrTypeFactory, memoria::RootCtr)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::btree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::btree::checksName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::btree::InsertBatchName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::btree::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::btree::FindName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::models::idx_map::CtrApiName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::bstree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::bstree::FindName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::models::root::CtrApiName)

MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::RootCtr, memoria::btree::IteratorAPIName)



MEMORIA_EXTERN_BASIC_CONTAINER(SmallCtrTypeFactory, memoria::Map1)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::btree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::btree::checksName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::btree::InsertBatchName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::btree::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::btree::FindName)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::models::idx_map::CtrApiName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::bstree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::bstree::FindName)
MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::Map1, memoria::btree::IteratorAPIName)



MEMORIA_EXTERN_BASIC_CONTAINER(SmallCtrTypeFactory, memoria::Set1)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::checksName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::InsertBatchName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::FindName)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::models::idx_map::CtrApiName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::bstree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::bstree::FindName)
MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::IteratorAPIName)


MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::models::idx_map::ItrApiName)


MEMORIA_EXTERN_BASIC_CONTAINER(SmallCtrTypeFactory, memoria::VectorCtr)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::btree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::btree::checksName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::btree::InsertBatchName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::btree::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::btree::FindName)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::dynvector::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::dynvector::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::dynvector::InsertName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::dynvector::SeekName)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::models::array::ApiName)

MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::btree::IteratorAPIName)
MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::VectorCtr, memoria::dynvector::IteratorAPIName)



/* */



MEMORIA_TEMPLATE_EXTERN template class memoria::InMemAllocator<
                                                    SmallProfile<>,
                                                    BasicContainerCollectionCfg<SmallProfile<> >::Page,
                                                    EmptyType
                                                >;


}

#endif
