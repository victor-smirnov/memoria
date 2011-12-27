
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_CONTAINER_TYPES_COLLECTION_HPP
#define	_MEMORIA_CORE_CONTAINER_CONTAINER_TYPES_COLLECTION_HPP


#include <memoria/core/container/names.hpp>
#include <memoria/core/container/dispatcher.hpp>
#include <memoria/metadata/container.hpp>
#include <memoria/core/container/init.hpp>

namespace memoria {

using memoria::vapi::ContainerCollectionMetadata;

template <typename Profile, typename SelectorType, typename ContainerTypeName> class CtrTF;

template <typename Profile> class ContainerCollectionCfg;




template <typename Profile>
class ContainerTypesCollection {

	typedef typename ContainerCollectionCfg<Profile>::Types		ContainerTypesType;

	static ContainerCollectionMetadata* 	reflection_;

public:

	//FIXME: Some Containers doesn't build if this declaration is omitted;
	typedef ContainerDispatcher<
								Profile,
								typename ContainerTypesType::AllocatorType::AbstractAllocator,
								NullType
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


	template <typename, typename>
	friend class ContainerTypesCollectionMetadataInit;
};

template <typename Profile>
ContainerCollectionMetadata* ContainerTypesCollection<Profile>::reflection_ = NULL;

}

#endif
