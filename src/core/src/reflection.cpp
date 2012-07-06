
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/metadata/container.hpp>
#include <memoria/core/tools/strings.hpp>

namespace std {

ostream& operator<<(ostream& os, const memoria::vapi::IDValue& id) {
	os<<id.str();
	return os;
}

}

namespace memoria {
namespace vapi {




ContainerMetadataRepository::ContainerMetadataRepository(StringRef name, const MetadataList &content) : MetadataGroup(name, content), hash_(0)
{
    MetadataGroup::set_type() = Metadata::MODEL;

    for (UInt c = 0; c < content_.size(); c++)
    {
        if (content[c]->getTypeCode() == Metadata::MODEL)
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
	if (model_map_.find(model->hash()) == model_map_.end())
	{
		hash_ = hash_ + model->hash();

		model_map_[model->hash()] = model;

		for (Int d = 0; d < model->size(); d++)
		{
			Metadata* item = model->getItem(d);
			if (item->getTypeCode() == Metadata::PAGE)
			{
				PageMetadata *page = static_cast<PageMetadata*> (item);
				page_map_[page->hash()] = page;
			}
			else if (item->getTypeCode() == Metadata::MODEL)
			{
				process_model(static_cast<ContainerMetadata*> (item));
			}
			else {
				//exception
			}
		}
	}
}


PageMetadata* ContainerMetadataRepository::getPageMetadata(Int hashCode) const {
    PageMetadataMap::const_iterator i = page_map_.find(hashCode);
    if (i != page_map_.end())
    {
        return i->second;
    }
    else {
    	throw Exception(MEMORIA_SOURCE, SBuf()<<"Unknown page type hash code "<<hashCode);
    }
}


ContainerMetadata* ContainerMetadataRepository::getContainerMetadata(Int hashCode) const {
    ContainerMetadataMap::const_iterator i = model_map_.find(hashCode);
    if (i != model_map_.end())
    {
        return i->second;
    }
    else {
    	throw Exception(MEMORIA_SOURCE, SBuf()<<"Unknown model hash code "<<hashCode);
    }
}



PageMetadata::PageMetadata(StringRef name, const MetadataList &content, Int attributes, Int hash0, const IPageOperations* page_operations, Int page_size):
    MetadataGroup(name, content), attributes_(attributes)
{
	MetadataGroup::set_type() = Metadata::PAGE;
    hash_ = hash0;
    page_operations_ = page_operations;
    page_size_ = page_size;

    if (page_operations == NULL)
    {
        throw NullPointerException(MEMORIA_SOURCE, "Page size provider is not specified");
    }
}



}
}


