
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



#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/metadata/container.hpp>

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif


namespace memoria {
namespace v1 {

std::ostream& operator<<(std::ostream& os, const v1::IDValue& id) {
    os << id.str();
    return os;
}




ContainerMetadataRepository::ContainerMetadataRepository(U16StringRef name, const MetadataList &content):
        MetadataGroup(name, content), hash_(0)
{
    MetadataGroup::set_type() = Metadata::CONTAINER;

    for (size_t c = 0; c < content_.size(); c++)
    {
        if (content[c]->getTypeCode() == Metadata::CONTAINER)
        {
            ContainerMetadataPtr model = static_pointer_cast<ContainerMetadata> (content_[c]);
            process_model(model);
        }
        else {
            //exception;
        }
    }
}

void ContainerMetadataRepository::process_model(const ContainerMetadataPtr& model)
{
    if (model_map_.find(model->ctr_hash()) == model_map_.end())
    {
        hash_ = hash_ + model->ctr_hash();

        model_map_[model->ctr_hash()] = model;
        
        for (int32_t d = 0; d < model->size(); d++)
        {
            auto item = model->getItem(d);
            if (item->getTypeCode() == Metadata::PAGE)
            {
                PageMetadataPtr page = static_pointer_cast<PageMetadata> (item);
                page_map_[page->hash() ^ model->ctr_hash()] = page;
            }
            else if (item->getTypeCode() == Metadata::CONTAINER)
            {
                process_model(static_pointer_cast<ContainerMetadata> (item));
            }
            else {
                //exception ?
            }
        }
    }
}


const PageMetadataPtr& ContainerMetadataRepository::getPageMetadata(uint64_t model_hash, uint64_t page_hash) const
{
    PageMetadataMap::const_iterator i = page_map_.find(model_hash ^ page_hash);
    if (i != page_map_.end())
    {
        return i->second;
    }
    else {
        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unknown page type hash codes {} {}", model_hash, page_hash));
    }
}


const ContainerMetadataPtr& ContainerMetadataRepository::getContainerMetadata(uint64_t hashCode) const
{
    auto i = model_map_.find(hashCode);
    if (i != model_map_.end())
    {
        return i->second;
    }
    else {
        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unknown container hash code {}", hashCode));
    }
}


void ContainerMetadataRepository::dumpMetadata(std::ostream& out)
{
    for (auto pair: model_map_)
    {
        if (pair.second->getCtrInterface() != nullptr)
        {
            out << pair.first << ": " << pair.second->getCtrInterface()->ctr_type_name() << std::endl;
        }
        else {
            out << pair.first << ": " << "Composite" << std::endl;
        }
    }
}


PageMetadata::PageMetadata(
                U16StringRef name,
                int32_t attributes,
                uint64_t hash,
                const IPageOperations* page_operations
              ):
    MetadataGroup(name)
{
    MetadataGroup::set_type() = Metadata::PAGE;
    hash_ = hash;
    page_operations_ = page_operations;

    if (!page_operations)
    {
        MMA1_THROW(NullPointerException()) << WhatInfo("Page operations is not specified");
    }
}



}}
