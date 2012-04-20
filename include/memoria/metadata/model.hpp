
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_MODEL_HPP
#define _MEMORIA_VAPI_METADATA_MODEL_HPP

#include <memoria/metadata/group.hpp>
#include <memoria/metadata/page.hpp>

namespace memoria    {
namespace vapi       {

struct ContainerInterface {
	virtual bool Check(const void* id, void* allocator) const	= 0;

	virtual ~ContainerInterface() {}
};

struct MEMORIA_API ContainerMetadata: public MetadataGroup {
public:

	ContainerMetadata(StringRef name, const MetadataList &content, Int code, ContainerInterface* container_interface):
		MetadataGroup(name, content),
		container_interface_(container_interface),
		code_(code),
		hash_(code_)
	{
		MetadataGroup::set_type() = MetadataGroup::MODEL;
		for (UInt c = 0; c < content.size(); c++)
	    {
	        if (content[c]->GetTypeCode() == Metadata::PAGE)
	        {
	            PageMetadata *page = static_cast<PageMetadata*> (content[c]);
	            page_map_[page->Hash()] = page;
	            hash_ += page->Hash() + code;
	        }
	        else {
	            //exception;
	        }
	    }
	}

	virtual ~ContainerMetadata() throw () {}

	virtual Int Hash() const {
		return hash_;
	}

	virtual Int Code() const {
		return code_;
	}

	virtual PageMetadata* GetPageMetadata(Int hashCode) const
	{
		PageMetadataMap::const_iterator i = page_map_.find(hashCode);
		if (i != page_map_.end()) {
			return i->second;
		}
		else {
			throw MemoriaException(MEMORIA_SOURCE, "Unknown page type hash code");
		}
	}

	virtual ContainerInterface* GetCtrInterface() const
	{
		return container_interface_;
	}

private:

    PageMetadataMap     	page_map_;
    ContainerInterface* 	container_interface_;

    Int 					code_;
    Int 					hash_;
};



}}

#endif
