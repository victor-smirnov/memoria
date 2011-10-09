
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_METADATA_HPP
#define _MEMORIA_CORE_API_METADATA_METADATA_HPP

#include <memoria/metadata/metadata.hpp>

#include <memoria/core/types/traits.hpp>
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/typemap.hpp>
#include <memoria/core/tools/bitmap.hpp>




namespace memoria { namespace vapi {

using memoria::TL;
using memoria::TLTool;
using memoria::Type2TypeMap;
using memoria::ValueTraits;

using memoria::CShr;


template <typename Interface>
class MetadataImplT: public Interface {
    typedef MetadataImplT<Interface>		Me;
    typedef Interface           Base;

public:

    MetadataImplT(StringRef name, Byte type): name_(name), typeCode_(type) {}

    virtual ~MetadataImplT() throw () {}

    StringRef Name() const {
        return name_;
    }

    virtual Int GetTypeCode() const {
        return typeCode_;
    }

    virtual bool IsGroup() const
    {
    	if (this->GetTypeCode() == Metadata::GROUP ||
    		this->GetTypeCode() == Metadata::PAGE  ||
    		this->GetTypeCode() == Metadata::MODEL ||
    		this->GetTypeCode() == Metadata::CONTAINER)
    	{
    		return true;
    	}
    	else {
    		return false;
    	}
    }

    virtual bool IsField() const {
    	return !IsGroup();
    }

    virtual bool IsNumber() const {
    	return (!IsGroup()) && this->GetTypeCode() != Metadata::ID && this->GetTypeCode() != Metadata::BIGINT && this->GetTypeCode() != Metadata::FLAG;
    }



protected:

    Int &set_type() {
        return typeCode_;
    }

private:
    const String    name_;
    Int             typeCode_;
};



typedef MetadataImplT<Metadata> 						MetadataImpl;

}}


#endif
