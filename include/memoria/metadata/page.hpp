
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


struct IPageLayoutEventHandler {

    virtual void startPage(const char* name)                                        = 0;
    virtual void endPage()                                                          = 0;

    virtual void startGroup(const char* name, Int elements = -1)                    = 0;
    virtual void endGroup()                                                         = 0;

    virtual void Layout(const char* name, Int type, Int ptr, Int size, Int count)   = 0;
};

struct IPageDataEventHandler {

    enum {BYTE_ARRAY, BITMAP};

    virtual ~IPageDataEventHandler() {}

    virtual void startPage(const char* name)                                                    = 0;
    virtual void endPage()                                                                      = 0;

    virtual void startLine(const char* name, Int size = -1)                                     = 0;
    virtual void endLine()                                                                      = 0;

    virtual void startGroup(const char* name, Int elements = -1)                                = 0;
    virtual void endGroup()                                                                     = 0;

    virtual void value(const char* name, const Byte* value, Int count = 1, Int kind = 0)        = 0;
    virtual void value(const char* name, const UByte* value, Int count = 1, Int kind = 0)       = 0;
    virtual void value(const char* name, const Short* value, Int count = 1, Int kind = 0)       = 0;
    virtual void value(const char* name, const UShort* value, Int count = 1, Int kind = 0)      = 0;
    virtual void value(const char* name, const Int* value, Int count = 1, Int kind = 0)         = 0;
    virtual void value(const char* name, const UInt* value, Int count = 1, Int kind = 0)        = 0;
    virtual void value(const char* name, const BigInt* value, Int count = 1, Int kind = 0)      = 0;
    virtual void value(const char* name, const UBigInt* value, Int count = 1, Int kind = 0)     = 0;
    virtual void value(const char* name, const IDValue* value, Int count = 1, Int kind = 0)     = 0;
};

struct DataEventsParams {};
struct LayoutEventsParams {};

struct IPageOperations
{
    virtual Int serialize(const void* page, void* buf) const                                    = 0;
    virtual void deserialize(const void* buf, Int buf_size, void* page) const                   = 0;
    virtual Int getPageSize(const void *page) const                                             = 0;

    virtual void resize(const void* page, void* buffer, Int new_size) const 					= 0;

    virtual void generateDataEvents(
                    const void* page,
                    const DataEventsParams& params,
                    IPageDataEventHandler* handler) const                                       = 0;

    virtual void generateLayoutEvents(
                    const void* page,
                    const LayoutEventsParams& params,
                    IPageLayoutEventHandler* handler) const                                     = 0;
};


struct MEMORIA_API PageMetadata: public MetadataGroup
{
    PageMetadata(
            StringRef name,
            Int attributes,
            Int hash0,
            const IPageOperations* page_operations);

    virtual ~PageMetadata() throw () {}

    virtual Int hash() const {
        return hash_;
    }


    virtual const IPageOperations* getPageOperations() const {
        return page_operations_;
    }

private:
    Int  hash_;
    bool attributes_;

    const IPageOperations* page_operations_;
};



}}

#endif
