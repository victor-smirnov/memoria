
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_CONTAINER_TYPES_COLLECTION_HPP
#define	_MEMORIA_CORE_CONTAINER_CONTAINER_TYPES_COLLECTION_HPP


#include <memoria/core/container/names.hpp>
#include <memoria/core/container/dispatcher.hpp>
#include <memoria/metadata/container.hpp>

namespace memoria {

using memoria::vapi::ContainerCollectionMetadata;

template <typename Profile, typename SelectorType, typename ContainerTypeName = SelectorType> class CtrTF;

template <typename Profile> class ContainerCollectionCfg;

template <typename Profile, typename List, typename Result = NullType> class ContainerName2TypeMapBuilder;

template <typename Profile, typename Head, typename Tail, typename ResultList>
class ContainerName2TypeMapBuilder<Profile, TL<Head, Tail>, ResultList> {
	typedef typename CtrTF<Profile, Head>::Type                 Type;
    typedef TL<Pair<Head, Type>, ResultList>                              NewResult;
public:
    typedef typename ContainerName2TypeMapBuilder<Profile, Tail, NewResult>::Result Result;
};

template <typename Profile, typename FinalResult>
class ContainerName2TypeMapBuilder<Profile, NullType, FinalResult> {
public:
    typedef FinalResult                                                         Result;
};


template <typename Profile>
class ContainerTypesCollection {

	typedef typename ContainerCollectionCfg<Profile>::Types		ContainerTypesType;

	typedef typename ContainerName2TypeMapBuilder<
								Profile,
								typename ContainerTypesType::RootNamesList
					 >::Result   								Name2TypeMapList;

	Name2TypeMapList list_;

	static ContainerCollectionMetadata* 	reflection_;

public:
	typedef ContainerDispatcher<
								Profile,
								typename ContainerTypesType::AllocatorType::AbstractAllocator,
								Name2TypeMapList
			>         											ContainerDispatcherType;


	template <typename ContainerName>
	struct Factory {
		typedef typename CtrTF<Profile, ContainerName>::Type 	Type;
	};


	template <typename ContainerName>
	struct Factory0 {
		typedef CtrTF<Profile, ContainerName>	Type;
	};

	template <typename ContainerName>
	struct Types {
		typedef typename CtrTF<Profile, ContainerName>::Types 	Type;
	};

	typedef typename Factory<Root>::Type						RootMapType;

	static ContainerCollectionMetadata* metadata() {
		return reflection_;
	}

	static Int Init()
	{
		if (reflection_ == NULL) {
			MetadataList list;
			ContainerDispatcherType::BuildMetadataList(list);
			reflection_ = new ContainerCollectionMetadataImpl(TypeNameFactory<Profile>::name(), list);
		}
		return reflection_->Hash();
	}

	static void PrintContainerHashes() {
		ContainerDispatcherType::PrintContainerHashes();
	}


};

template <typename Profile>
ContainerCollectionMetadata* ContainerTypesCollection<Profile>::reflection_ = NULL;

}

#endif
