
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/container/dispatcher.hpp>
#include <memoria/v1/metadata/container.hpp>
#include <memoria/v1/containers/roots.hpp>

#include <memoria/v1/core/types/algo/select.hpp>

namespace memoria {
namespace v1 {

using v1::ContainerMetadataRepository;

template <typename Profile, typename SelectorType, typename ContainerTypeName = SelectorType> class CtrTF;

template <typename Profile>
class MetadataRepository {
    static ContainerMetadataRepository* metadata_;
public:

    static ContainerMetadataRepository* getMetadata()
    {
        return metadata_;
    }

    static void registerMetadata(const ContainerMetadataPtr& ctr_metadata)
    {
        metadata_->registerMetadata(ctr_metadata);
    }

    static void unregisterMetadata(const ContainerMetadataPtr& ctr_metadata)
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



}}
