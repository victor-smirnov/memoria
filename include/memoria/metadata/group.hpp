
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_GROUP_HPP
#define _MEMORIA_VAPI_METADATA_GROUP_HPP

#include <memoria/metadata/metadata.hpp>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API MetadataGroup: public Metadata {
    virtual Int Size() const   = 0;
    virtual Metadata* GetItem(Int idx) const  = 0;
    virtual Metadata* FindFirst(const char* name, bool throwEx = false) = 0;

    
};

inline bool isGroup(Metadata *meta) {
    Int type = meta->GetTypeCode();
    return type == Metadata::GROUP || type == Metadata::CONTAINER || type == Metadata::MODEL ||
        type == Metadata::PAGE;
}



}}

#endif
