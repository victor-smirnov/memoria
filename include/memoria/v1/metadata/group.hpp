
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

#include <memoria/v1/metadata/metadata.hpp>

namespace memoria {
namespace v1 {

struct MetadataGroup: public Metadata {
public:

    MetadataGroup(StringRef name, const MetadataList &content, Int block_size = 0, Int type = Metadata::GROUP):
        Metadata(name, type), content_(content.size()), block_size_(block_size)
    {
        for (UInt c = 0; c < content.size(); c++) {
            content_[c] = content[c];
        }
    }

    MetadataGroup(StringRef name, Int block_size = 0, Int type = Metadata::GROUP):
        Metadata(name, type), block_size_(block_size)
    {}

    virtual ~MetadataGroup() throw ()
    {}

    virtual Int size() const {
        return content_.size();
    }

    virtual const MetadataPtr& getItem(Int idx) const {
        return content_[idx];
    }

    virtual void putAll(MetadataList& target) const
    {
        for (auto i = content_.begin(); i != content_.end(); i++)
        {
            target.push_back(*i);
        }
    }

    virtual Int getBlockSize() const
    {
        return block_size_;
    }

protected:
    MetadataList    content_;
    Int             block_size_;

};



inline bool isGroup(Metadata *meta)
{
    Int type = meta->getTypeCode();
    return type == Metadata::GROUP || type == Metadata::CONTAINER || type == Metadata::PAGE;
}



}}
