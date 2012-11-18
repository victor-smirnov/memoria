
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_METADATA_HPP1
#define _MEMORIA_VAPI_METADATA_METADATA_HPP1

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/traits.hpp>
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/typemap.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/tools/id.hpp>
#include <memoria/core/tools/config.hpp>

#include <vector>
#include <unordered_map>
#include <map>
#include <string>

namespace memoria    {
namespace vapi       {

struct Metadata;
struct PageMetadata;
struct ContainerMetadata;
struct ContainerCollection;
struct Container;

typedef std::vector<Metadata*>                          MetadataList;
typedef std::map<Int, PageMetadata*>                    PageMetadataMap;
typedef std::map<Int, ContainerMetadata*>               ContainerMetadataMap;

struct MEMORIA_API Metadata {

public:
    enum   {BYTE,   UBYTE,  SHORT,   USHORT, INT,    UINT,
        BIGINT, ID,     BITMAP,  FLAG,   GROUP,  PAGE,
        MODEL,  CONTAINER, MAP};

    typedef Metadata            Me;

public:

    Metadata(StringRef name, Byte type): name_(name), typeCode_(type) {}
    virtual ~Metadata() throw () {}

    StringRef name() const {
        return name_;
    }

    virtual Int getTypeCode() const {
        return typeCode_;
    }

    virtual bool isGroup() const
    {
        if (this->getTypeCode() == Metadata::GROUP ||
                this->getTypeCode() == Metadata::PAGE  ||
                this->getTypeCode() == Metadata::MODEL ||
                this->getTypeCode() == Metadata::MAP   ||
                this->getTypeCode() == Metadata::CONTAINER)
        {
            return true;
        }
        else {
            return false;
        }
    }

    virtual bool isField() const {
        return !isGroup();
    }

    virtual bool isNumber() const {
        return (!isGroup())
                && this->getTypeCode() != Metadata::ID
                && this->getTypeCode() != Metadata::BIGINT
                && this->getTypeCode() != Metadata::FLAG;
    }



protected:

    Int &set_type() {
        return typeCode_;
    }

private:
    const String    name_;
    Int             typeCode_;
};



}}

#endif
