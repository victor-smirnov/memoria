
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

//static Int BTREE        = 1;
//static Int ROOT         = 2;
//static Int LEAF         = 4;
//static Int BITMAP       = 8;

struct MEMORIA_API PageMetadata: public MetadataGroup {
    virtual Int Hash() const												= 0;
    virtual bool IsAbiCompatible() const									= 0;

    virtual void Externalize(const void *mem, void *buf) const				= 0;
    virtual void Internalize(const void *buf, void *mem, Int size) const	= 0;

    virtual Int GetDataBlockSize(const void* mem) const						= 0;
    virtual Int GetPageDataBlockSize(const Page* tpage) const				= 0;
    virtual const FieldMetadata* GetField(Int ptr, bool abi) const			= 0;

    virtual Int GetPageSize() const											= 0;

    virtual FieldMetadata* GetLastField() const								= 0;
};



}}

#endif
