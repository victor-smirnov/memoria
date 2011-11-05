
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_MODEL_HPP
#define _MEMORIA_VAPI_METADATA_MODEL_HPP

#include <memoria/metadata/group.hpp>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API ContainerMetadata: public MetadataGroup {
    virtual Int Hash() const									= 0;
    virtual Int Code() const 									= 0;

    virtual PageMetadata* GetPageMetadata(Int hashCode) const 	= 0;
};



}}

#endif
