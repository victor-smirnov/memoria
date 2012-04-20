
// Copyright Victor Smirnov 2011, 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_CONTAINER_TYPES_COLLECTION_HPP
#define	_MEMORIA_CORE_CONTAINER_CONTAINER_TYPES_COLLECTION_HPP


#include <memoria/core/container/names.hpp>
#include <memoria/core/container/dispatcher.hpp>
#include <memoria/metadata/container.hpp>
#include <memoria/containers/roots.hpp>

namespace memoria {

using memoria::vapi::ContainerMetadataRepository;

template <typename Profile, typename SelectorType, typename ContainerTypeName = SelectorType> class CtrTF;

template <typename Profile>
struct CtrTypeFactory {
	template <typename ContainerName>
	struct Types {
		typedef typename CtrTF<Profile, ContainerName>::Types 	Type;
	};

	template <typename ContainerName>
	struct Factory {
		typedef typename CtrTF<Profile, ContainerName>::Type 	Type;
	};

	template <typename ContainerName>
	struct Factory0 {
		typedef CtrTF<Profile, ContainerName>					Type;
	};
};



template <typename Profile>
class MetadataRepository {
	static ContainerMetadataRepository* metadata_;
public:

	static ContainerMetadataRepository* GetMetadata()
	{
		return metadata_;
	}

	static void Register(ContainerMetadata* ctr_metadata)
	{
		metadata_->Register(ctr_metadata);
	}

	static void Unregister(ContainerMetadata* ctr_metadata)
	{
		metadata_->Unregister(ctr_metadata);
	}

	static void Init()
	{
		if (metadata_ == NULL)
		{
			metadata_ = new ContainerMetadataRepository(TypeNameFactory<Profile>::name(), MetadataList());
		}
	}
};

template <typename Profile>
ContainerMetadataRepository* MetadataRepository<Profile>::metadata_ = NULL;


template <typename ProfileList = ::memoria::ProfileListBuilder<>::Type >
class Memoria {
public:
	static Int Init()
	{
		MetadataRepository<typename ProfileList::Head>::Init();
		return Memoria<typename ProfileList::Tail>::Init();
	}
};

template <>
class Memoria<NullType> {
public:
	static Int Init() {
		return 1;
	}
};


#define MEMORIA_INIT()													\
const int MEMORIA_INITIALIZED = ::memoria::Memoria<>::Init()






}

#endif
