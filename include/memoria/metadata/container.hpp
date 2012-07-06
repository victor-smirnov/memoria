
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_CONTAINER_HPP
#define _MEMORIA_VAPI_METADATA_CONTAINER_HPP

#include <memoria/metadata/group.hpp>
#include <memoria/metadata/model.hpp>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API ContainerMetadataRepository: public MetadataGroup {

	public:

		ContainerMetadataRepository(StringRef name, const MetadataList &content);

		virtual ~ContainerMetadataRepository() throw () {}

		virtual Int Hash() const {
			return hash_;
		}

		PageMetadata* getPageMetadata(Int hashCode) const;
		ContainerMetadata* getContainerMetadata(Int hashCode) const;


		virtual void registerMetadata(ContainerMetadata* metadata)
		{
			process_model(metadata);
		}

		virtual void unregisterMetadata(ContainerMetadata* metadata) {}


	private:
	    Int                 	hash_;
	    PageMetadataMap     	page_map_;
	    ContainerMetadataMap    model_map_;

	    void process_model(ContainerMetadata* model);
};










}}


#endif
