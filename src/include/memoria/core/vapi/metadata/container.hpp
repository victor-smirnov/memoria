
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_CONTAINER_HPP
#define _MEMORIA_CORE_API_METADATA_CONTAINER_HPP

#include <memoria/metadata/container.hpp>
#include <memoria/core/vapi/metadata/group.hpp>

#include <memoria/core/vapi/metadata/page.hpp>
#include <memoria/core/vapi/metadata/model.hpp>

#include <string>



namespace memoria { namespace vapi {

template <typename Interface>
class ContainerCollectionMetadataImplT: public MetadataGroupImplT<Interface> {
	typedef ContainerCollectionMetadataImplT<Interface> 		Me;
	typedef MetadataGroupImplT<Interface> 			Base;
public:

	ContainerCollectionMetadataImplT(StringRef name, const MetadataList &content);

	virtual ~ContainerCollectionMetadataImplT() throw () {}

	virtual Int Hash() const {
		return hash_;
	}

	PageMetadata* GetPageMetadata(Int hashCode) const;
	ContainerMetadata* GetContainerMetadata(Int hashCode) const;





private:
    Int                 hash_;
    PageMetadataMap     page_map_;
    ContainerMetadataMap    model_map_;
};



typedef ContainerCollectionMetadataImplT<ContainerCollectionMetadata> 					ContainerCollectionMetadataImpl;



template <typename Interface>
ContainerCollectionMetadataImplT<Interface>::ContainerCollectionMetadataImplT(StringRef name, const MetadataList &content) : Base(name, content), hash_(0) {
    Base::set_type() = Metadata::MODEL;

    for (UInt c = 0; c < content.size(); c++)
    {
        if (content[c]->GetTypeCode() == Metadata::MODEL)
        {
            ContainerMetadata *model = static_cast<ContainerMetadata*> (content[c]);
            hash_ = hash_ + model->Hash();

            model_map_[model->Hash()] = model;


            for (Int d = 0; d < model->Size(); d++)
            {
                PageMetadata *page = static_cast<PageMetadata*> (model->GetItem(d));
                page_map_[page->Hash()] = page;
            }
        }
        else {
            //exception;
        }
    }
}

template <typename Interface>
PageMetadata* ContainerCollectionMetadataImplT<Interface>::GetPageMetadata(Int hashCode) const {
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
ContainerMetadata* ContainerCollectionMetadataImplT<Interface>::GetContainerMetadata(Int hashCode) const {
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
