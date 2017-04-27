
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

public:

    static ContainerMetadataRepository* getMetadata()
    {
    	static thread_local ContainerMetadataRepository metadata(TypeNameFactory<Profile>::name(), MetadataList());
        return &metadata;
    }

    static void registerMetadata(const ContainerMetadataPtr& ctr_metadata)
    {
        getMetadata()->registerMetadata(ctr_metadata);
    }

    static void unregisterMetadata(const ContainerMetadataPtr& ctr_metadata)
    {
        getMetadata()->unregisterMetadata(ctr_metadata);
    }
};


#define MEMORIA_INIT(Profile)



}}
