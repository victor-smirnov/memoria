
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
    typedef BasicContainerCollectionCfg<SmallProfile<T> >                      			Types;
};


typedef memoria::InMemAllocator<SmallProfile<>, ContainerCollectionCfg<SmallProfile<> >::Types::Page, EmptyType> SmallInMemAllocator;
typedef CtrTypeFactory<SmallProfile<> > SmallCtrTypeFactory;
MEMORIA_TEMPLATE_EXTERN template class MetadataRepository<SmallProfile<> >;


//MEMORIA_EXTERN_BASIC_CONTAINER(StreamContainerTypesCollection, memoria::Root)
//
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::ToolsName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::ChecksName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::InsertName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::RemoveName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::FindName)
//
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::models::kvmap::MapApiName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::models::kvmap::FindName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::models::kvmap::ChecksName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::models::kvmap::InsertName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::models::kvmap::RemoveName)
//
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::models::TreeMapName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::models::MapName)
//
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::IteratorToolsName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::IteratorWalkName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::IteratorAPIName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::IteratorMultiskipName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Root, memoria::btree::IteratorContainerAPIName)


//MEMORIA_EXTERN_BASIC_CONTAINER(StreamContainerTypesCollection, memoria::SumMap1)
//
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::ToolsName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::StubsName)
//
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::ChecksName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::InsertName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::InsertBatchName)
//
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::RemoveName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::FindName)
//
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::models::idx_map::RemoveName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::bstree::ToolsName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::models::idx_map::InsertName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::bstree::FindName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::models::idx_map::ContainerApiName)
//
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::models::TreeMapName)
//MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::models::MapName)
//
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::IteratorToolsName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::IteratorWalkName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::IteratorAPIName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::IteratorMultiskipName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::btree::IteratorContainerAPIName)
//
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::bstree::IteratorToolsName)
//MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::SumMap1, memoria::models::idx_map::ItrAPIName)




MEMORIA_EXTERN_BASIC_CONTAINER(SmallCtrTypeFactory, memoria::Set1)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::ChecksName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::InsertBatchName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::FindName)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::models::idx_map::CtrApiName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::bstree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::bstree::FindName)
MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::btree::IteratorAPIName)


MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::Set1, memoria::models::idx_map::ItrApiName)


MEMORIA_EXTERN_BASIC_CONTAINER(SmallCtrTypeFactory, memoria::Vector)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::btree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::btree::ChecksName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::btree::InsertBatchName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::btree::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::btree::FindName)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::dynvector::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::dynvector::RemoveName)

MEMORIA_EXTERN_CTR_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::dynvector::Insert2Name)

MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::btree::IteratorAPIName)
MEMORIA_EXTERN_ITER_PAPRT(SmallCtrTypeFactory, memoria::Vector, memoria::dynvector::IteratorAPIName)



/* */



MEMORIA_TEMPLATE_EXTERN template class memoria::InMemAllocator<SmallProfile<>, BasicContainerCollectionCfg<SmallProfile<> >::Page, EmptyType>;


}

#endif
