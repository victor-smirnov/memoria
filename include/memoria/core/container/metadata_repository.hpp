
// Copyright Victor Smirnov 2011, 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_CONTAINER_TYPES_COLLECTION_HPP
#define _MEMORIA_CORE_CONTAINER_CONTAINER_TYPES_COLLECTION_HPP


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
        typedef typename CtrTF<Profile, ContainerName>::Types   Type;
    };

    template <typename ContainerName>
    struct Factory {
        typedef typename CtrTF<Profile, ContainerName>::Type    Type;
    };

    template <typename ContainerName>
    struct Factory0 {
        typedef CtrTF<Profile, ContainerName>                   Type;
    };
};



template <typename Profile>
class MetadataRepository {
    static ContainerMetadataRepository* metadata_;
public:

    static ContainerMetadataRepository* getMetadata()
    {
        return metadata_;
    }

    static void registerMetadata(ContainerMetadata* ctr_metadata)
    {
        metadata_->registerMetadata(ctr_metadata);
    }

    static void unregisterMetadata(ContainerMetadata* ctr_metadata)
    {
        metadata_->unregisterMetadata(ctr_metadata);
    }

    static void init()
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
    static Int init()
    {
        MetadataRepository<typename ProfileList::Head>::init();
        return Memoria<typename ProfileList::Tail>::init();
    }
};

template <>
class Memoria<NullType> {
public:
    static Int init() {
        return 1;
    }
};


#define MEMORIA_INIT()                                                  \
const int MEMORIA_INITIALIZED = ::memoria::Memoria<>::init()




template <
	template <int> class Decl,
	int Value = 100,
	typename List = NullType
>
class SimpleOrderedBuilder {
	typedef typename Decl<Value>::Type 							DeclType;

	typedef typename IfThenElse<
		IfTypesEqual<DeclType, NotDefined>::Value,
		List,
		typename AppendTool<DeclType, List>::Result
	>::Result 													NewList;

public:
	typedef typename SimpleOrderedBuilder<Decl, Value - 1, NewList>::Type 	Type;
};

template <
	template <int> class Decl,
	typename List
>
class SimpleOrderedBuilder<Decl, -1, List> {
public:
	typedef List Type;
};

typedef SimpleOrderedBuilder<CtrNameDeclarator> CtrNameListBuilder;


template <typename ProfileType, typename NameList>
struct CtrListInitializer {
	static void init() {
		CtrTF<ProfileType, typename NameList::Head>::Type::initMetadata();

		CtrListInitializer<ProfileType, typename NameList::Tail>::init();
	}
};

template <typename ProfileType>
struct CtrListInitializer<ProfileType, NullType> {
	static void init() {}
};

template <
	typename Profile,

	template <
		template <int> class Decl,
		int Value,
		typename List
	>
	class CtrListBuilder = SimpleOrderedBuilder
>
class MetadataInitializer {
	typedef typename CtrListBuilder<CtrNameDeclarator, 100, NullType>::Type CtrNameList;

public:
	static void init() {
		CtrListInitializer<Profile, CtrNameList>::init();
	}
};





}

#endif
