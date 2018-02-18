
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/types/traits.hpp>
#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/types/typemap.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/core/tools/id.hpp>
#include <memoria/v1/core/tools/config.hpp>

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace memoria {
namespace v1 {

struct Metadata;
using MetadataPtr = CtrSharedPtr<Metadata>;

struct PageMetadata;
using PageMetadataPtr = CtrSharedPtr<PageMetadata>;

struct ContainerMetadata;
using ContainerMetadataPtr = CtrSharedPtr<ContainerMetadata>;

struct ContainerCollection;
struct Container;

// FIXME change map key to uint32_t
using MetadataList          = std::vector<MetadataPtr>;
using PageMetadataMap       = std::unordered_map<uint64_t, PageMetadataPtr>;
using ContainerMetadataMap  = std::unordered_map<uint64_t, ContainerMetadataPtr>;

struct Metadata {

public:
    enum   {GROUP,  PAGE, CONTAINER};

    using Me = Metadata;

public:

    Metadata(U16StringRef name, int8_t type): name_(name), typeCode_(type) {}
    virtual ~Metadata() throw () {}

    U16StringRef name() const {
        return name_;
    }

    virtual int32_t getTypeCode() const {
        return typeCode_;
    }

    virtual bool isGroup() const
    {
        if (this->getTypeCode() == Metadata::GROUP ||
                this->getTypeCode() == Metadata::PAGE  ||
                this->getTypeCode() == Metadata::CONTAINER)
        {
            return true;
        }
        else {
            return false;
        }
    }

protected:

    int32_t &set_type() {
        return typeCode_;
    }

private:
    const U16String  name_;
    int32_t          typeCode_;
};



}}
