
// Copyright Victor Smirnov 2011- 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/container/dispatcher.hpp>
#include <memoria/v1/metadata/container.hpp>
#include <memoria/v1/containers/roots.hpp>

#include <memoria/v1/core/types/algo/select.hpp>

namespace memoria {

using memoria::ContainerMetadataRepository;

template <typename Profile, typename SelectorType, typename ContainerTypeName = SelectorType> class CtrTF;

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

    static void cleanup() {
        if (metadata_) {
            delete metadata_;
            metadata_ = nullptr;
        }
    }
};

template <typename Profile>
ContainerMetadataRepository* MetadataRepository<Profile>::metadata_ = NULL;




template <
    template <int> class Decl,
    int Value = 100,
    typename List = TypeList<>
>
class SimpleOrderedBuilder {
    typedef typename Decl<Value>::Type                          DeclType;

    typedef IfThenElse<
        IfTypesEqual<DeclType, NotDefined>::Value,
        List,
        typename AppendTool<DeclType, List>::Type
    >                                                           NewList;

public:
    typedef typename SimpleOrderedBuilder<Decl, Value - 1, NewList>::Type   Type;
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
        CtrTF<ProfileType, typename ListHead<NameList>::Type>::Type::initMetadata();

        CtrListInitializer<ProfileType, typename ListTail<NameList>::Type>::init();
    }
};

template <typename ProfileType>
struct CtrListInitializer<ProfileType, TypeList<> > {
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
    typedef typename CtrListBuilder<CtrNameDeclarator, 100, TypeList<> >::Type CtrNameList;

public:
    static void init() {
        MetadataRepository<Profile>::init();

        CtrListInitializer<Profile, CtrNameList>::init();
    }
};


#define MEMORIA_INIT(Profile)\
    MetadataInitializer<Profile>::init()



}
