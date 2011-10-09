
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_CONTAINER_HPP
#define _MEMORIA_VAPI_METADATA_CONTAINER_HPP

#include <memoria/metadata/group.hpp>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API ContainerCollectionMetadata: public MetadataGroup {
    virtual Int Hash() const										= 0;

    virtual PageMetadata* GetPageMetadata(Int hashCode) const 		= 0;
    virtual ContainerMetadata* GetContainerMetadata(Int hashCode) const 	= 0;

    
};

}}


#endif
