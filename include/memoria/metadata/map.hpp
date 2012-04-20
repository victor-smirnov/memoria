
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_MAP_HPP
#define _MEMORIA_VAPI_METADATA_MAP_HPP

#include <memoria/metadata/field.hpp>
#include <memoria/metadata/group.hpp>


namespace memoria    {
namespace vapi       {

struct MEMORIA_API MapMetadata: public MetadataGroup {

    virtual Int GetSize(const void* mem) const 		= 0;
    virtual Int GetIndexSize(const void* mem) const	= 0;
    virtual Int GetBlocks()	const					= 0;

    virtual Int GetKeySize() const					= 0;
    virtual Int GetIndexKeySize() const				= 0;
    virtual Int GetValueSize() const				= 0;

    virtual bool IsSet() const						= 0;

    virtual FieldMetadata* GetIndexField(const void* mem, Int block_num, Int idx) 	= 0;
    virtual FieldMetadata* GetKeyField(const void* mem, Int block_num, Int idx) 	= 0;
    virtual FieldMetadata* GetValueField(const void* mem, Int idx) 					= 0;
};


}}

#endif
