
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/traits.hpp>
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/typemap.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/tools/id.hpp>
#include <memoria/core/tools/config.hpp>

#include <vector>
#include <unordered_map>
#include <string>

namespace memoria    {

struct Metadata;
struct PageMetadata;
struct ContainerMetadata;
struct ContainerCollection;
struct Container;

typedef std::vector<Metadata*>                          MetadataList;
typedef std::unordered_map<Int, PageMetadata*>          PageMetadataMap;
typedef std::unordered_map<Int, ContainerMetadata*>     ContainerMetadataMap;


struct MEMORIA_API Metadata {

public:
    enum   {GROUP,  PAGE, CONTAINER};

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
                this->getTypeCode() == Metadata::CONTAINER)
        {
            return true;
        }
        else {
            return false;
        }
    }

protected:

    Int &set_type() {
        return typeCode_;
    }

private:
    const String    name_;
    Int             typeCode_;
};



}
