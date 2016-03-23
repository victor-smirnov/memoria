
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/core/tools/strings/string.hpp>
#include <memoria/v1/metadata/container.hpp>


namespace memoria {

ostream& operator<<(ostream& os, const memoria::IDValue& id) {
    os<<id.str();
    return os;
}




ContainerMetadataRepository::ContainerMetadataRepository(StringRef name, const MetadataList &content):
        MetadataGroup(name, content), hash_(0)
{
    MetadataGroup::set_type() = Metadata::CONTAINER;

    for (UInt c = 0; c < content_.size(); c++)
    {
        if (content[c]->getTypeCode() == Metadata::CONTAINER)
        {
            ContainerMetadata *model = static_cast<ContainerMetadata*> (content_[c]);
            process_model(model);
        }
        else {
            //exception;
        }
    }
}

void ContainerMetadataRepository::process_model(ContainerMetadata* model)
{
    if (model_map_.find(model->ctr_hash()) == model_map_.end())
    {
        hash_ = hash_ + model->ctr_hash();

        model_map_[model->ctr_hash()] = model;

        for (Int d = 0; d < model->size(); d++)
        {
            Metadata* item = model->getItem(d);
            if (item->getTypeCode() == Metadata::PAGE)
            {
                PageMetadata *page = static_cast<PageMetadata*> (item);
                page_map_[page->hash() ^ model->ctr_hash()] = page;
            }
            else if (item->getTypeCode() == Metadata::CONTAINER)
            {
                process_model(static_cast<ContainerMetadata*> (item));
            }
            else {
                //exception
            }
        }
    }
}


PageMetadata* ContainerMetadataRepository::getPageMetadata(Int model_hash, Int page_hash) const
{
    PageMetadataMap::const_iterator i = page_map_.find(model_hash ^ page_hash);
    if (i != page_map_.end())
    {
        return i->second;
    }
    else {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Unknown page type hash codes "<<model_hash<<" "<<page_hash);
    }
}


ContainerMetadata* ContainerMetadataRepository::getContainerMetadata(Int hashCode) const
{
    auto i = model_map_.find(hashCode);
    if (i != model_map_.end())
    {
        return i->second;
    }
    else {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Unknown model hash code "<<hashCode);
    }
}


void ContainerMetadataRepository::dumpMetadata(std::ostream& out)
{
    for (auto pair: model_map_)
    {
        if (pair.second->getCtrInterface() != nullptr)
        {
            out<<pair.first<<": "<<pair.second->getCtrInterface()->ctr_type_name()<<std::endl;
        }
        else {
            out<<pair.first<<": "<<"Composite"<<std::endl;
        }
    }
}


PageMetadata::PageMetadata(
                StringRef name,
                Int attributes,
                Int hash,
                const IPageOperations* page_operations
              ):
    MetadataGroup(name)
{
    MetadataGroup::set_type() = Metadata::PAGE;
    hash_ = hash;
    page_operations_ = page_operations;

    if (page_operations == NULL)
    {
        throw NullPointerException(MEMORIA_SOURCE, "Page operations is not specified");
    }
}



}



