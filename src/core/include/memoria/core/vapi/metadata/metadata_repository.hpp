
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_CONTAINER_HPP
#define _MEMORIA_CORE_API_METADATA_CONTAINER_HPP

#include <memoria/metadata/container.hpp>
#include <memoria/core/vapi/metadata/group.hpp>

#include <memoria/core/vapi/metadata/page.hpp>
#include <memoria/core/vapi/metadata/container.hpp>

#include <memoria/core/tools/strings.hpp>

#include <string>



namespace memoria { namespace vapi {

template <typename Interface>
class ContainerMetadataRepositoryImplT: public MetadataGroupImplT<Interface> {
	typedef ContainerMetadataRepositoryImplT<Interface> 		Me;
	typedef MetadataGroupImplT<Interface> 			Base;
public:

	ContainerMetadataRepositoryImplT(StringRef name, const MetadataList &content);

	virtual ~ContainerMetadataRepositoryImplT() throw () {}

	virtual Int Hash() const {
		return hash_;
	}

	PageMetadata* GetPageMetadata(Int hashCode) const;
	ContainerMetadata* GetContainerMetadata(Int hashCode) const;


	virtual void Register(ContainerMetadata* metadata)
	{
		process_model(metadata);
	}

	virtual void Unregister(ContainerMetadata* metadata) {}


private:
    Int                 	hash_;
    PageMetadataMap     	page_map_;
    ContainerMetadataMap    model_map_;

    void process_model(ContainerMetadata* model);
};



typedef ContainerMetadataRepositoryImplT<ContainerMetadataRepository> 					ContainerCollectionMetadataImpl;



template <typename Interface>
ContainerMetadataRepositoryImplT<Interface>::ContainerMetadataRepositoryImplT(StringRef name, const MetadataList &content) : Base(name, content), hash_(0) {
    Base::set_type() = Metadata::MODEL;

    for (UInt c = 0; c < content.size(); c++)
    {
        if (content[c]->GetTypeCode() == Metadata::MODEL)
        {
            ContainerMetadata *model = static_cast<ContainerMetadata*> (content[c]);
            process_model(model);
        }
        else {
            //exception;
        }
    }
}
template <typename Interface>
void ContainerMetadataRepositoryImplT<Interface>::process_model(ContainerMetadata* model)
{
	if (model_map_.find(model->Hash()) == model_map_.end())
	{
		hash_ = hash_ + model->Hash();

		model_map_[model->Hash()] = model;

		for (Int d = 0; d < model->Size(); d++)
		{
			Metadata* item = model->GetItem(d);
			if (item->GetTypeCode() == Metadata::PAGE)
			{
				PageMetadata *page = static_cast<PageMetadata*> (item);
				page_map_[page->Hash()] = page;
			}
			else if (item->GetTypeCode() == Metadata::MODEL)
			{
				process_model(static_cast<ContainerMetadata*> (item));
			}
			else {
				//exception
			}
		}
	}
}

template <typename Interface>
PageMetadata* ContainerMetadataRepositoryImplT<Interface>::GetPageMetadata(Int hashCode) const {
    PageMetadataMap::const_iterator i = page_map_.find(hashCode);
    if (i != page_map_.end())
    {
        return i->second;
    }
    else {
    	throw MemoriaException(MEMORIA_SOURCE, "Unknown page type hash code "+ToString(hashCode));
    }
}

template <typename Interface>
ContainerMetadata* ContainerMetadataRepositoryImplT<Interface>::GetContainerMetadata(Int hashCode) const {
    ContainerMetadataMap::const_iterator i = model_map_.find(hashCode);
    if (i != model_map_.end())
    {
        return i->second;
    }
    else {
    	throw MemoriaException(MEMORIA_SOURCE, "Unknown model hash code " + ToString(hashCode));
    }
}


}}


#endif
