
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_PAGE_HPP
#define _MEMORIA_VAPI_METADATA_PAGE_HPP

#include <memoria/metadata/group.hpp>

namespace memoria    {
namespace vapi       {

enum {BTREE = 1, ROOT = 2, LEAF = 4, BITMAP = 8};

struct IPageOperations
{
	virtual Int Serialize(const void* page, void* buf) const					= 0;
	virtual void Deserialize(const void* buf, Int buf_size, void* page) const	= 0;
	virtual Int GetPageSize(const void *page) const								= 0;
};


struct MEMORIA_API PageMetadata: public MetadataGroup
{
    virtual Int Hash() const												= 0;
    virtual const IPageOperations* GetPageOperations() const 				= 0;
};



}}

#endif
