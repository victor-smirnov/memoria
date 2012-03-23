
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

#include <memoria/core/container/checker.hpp>
#include <memoria/core/container/collection.hpp>

#include "names.hpp"
#include "allocator.hpp"

namespace memoria    {

using namespace memoria::btree;


template <typename Profile>
class ContainerCollectionCfg;

template <typename T>
class ContainerCollectionCfg<StreamProfile<T> > {

    struct StreamContainerCollectionCfg: public BasicContainerCollectionCfg<StreamProfile<T> > {
        typedef BasicContainerCollectionCfg<StreamProfile<T> > Base;

        typedef memoria::StreamAllocator<
        		StreamProfile<T>,
					typename Base::Page,
					typename Base::Transaction
				>  	 															AllocatorType;
    };

public:
    typedef StreamContainerCollectionCfg                      							Types;

};


typedef memoria::StreamAllocator<StreamProfile<>, BasicContainerCollectionCfg<StreamProfile<> >::Page, EmptyType> DefaultStreamAllocator;
typedef ContainerTypesCollection<StreamProfile<> > StreamContainerTypesCollection;
typedef Checker<StreamContainerTypesCollection, DefaultStreamAllocator> StreamContainersChecker;

MEMORIA_TEMPLATE_EXTERN template class ContainerTypesCollection<StreamProfile<> >;


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




MEMORIA_EXTERN_BASIC_CONTAINER(StreamContainerTypesCollection, memoria::Set1)

MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::btree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::btree::ChecksName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::btree::InsertBatchName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::btree::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::btree::FindName)

MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::models::idx_map::CtrApiName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::bstree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::bstree::FindName)
MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::btree::IteratorAPIName)


MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Set1, memoria::models::idx_map::ItrApiName)


MEMORIA_EXTERN_BASIC_CONTAINER(StreamContainerTypesCollection, memoria::Vector)

MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::btree::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::btree::ChecksName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::btree::InsertBatchName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::btree::RemoveName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::btree::FindName)

MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::dynvector::ToolsName)
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::dynvector::RemoveName)

//FIXME CtrPart
MEMORIA_EXTERN_CTR_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::dynvector::Insert2Name)

MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::btree::IteratorAPIName)
MEMORIA_EXTERN_ITER_PAPRT(StreamContainerTypesCollection, memoria::Vector, memoria::dynvector::IteratorAPIName)

/**/


#if !defined(MEMORIA_DLL) && !defined(MEMORIA_MAIN)

// This is a workaroud. GCC 4.5.1 can't build models without this builder.
//extern template class ContainerTypesHelper<StreamProfile<> >;

extern template class memoria::StreamAllocator<StreamProfile<>, BasicContainerCollectionCfg<StreamProfile<> >::Page, EmptyType>;
extern template class ContainerTypesCollection<StreamProfile<> >;
extern template class Checker<StreamContainerTypesCollection, DefaultStreamAllocator>;

#endif

}

#endif
