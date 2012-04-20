
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

typedef std::vector<Metadata*>          				MetadataList;
typedef std::map<Int, PageMetadata*>    				PageMetadataMap;
typedef std::map<Int, ContainerMetadata*>   			ContainerMetadataMap;


//typedef Container* (*ContainerFactoryFn) (const IDValue& rootId, ContainerCollection *container, BigInt name);
//typedef Int (*PageSizeProviderFn)(const void *page);

class MEMORIA_API Metadata {

public:
	enum   {BYTE,   UBYTE,  SHORT,   USHORT, INT,    UINT,
		BIGINT, ID,     BITMAP,  FLAG,   GROUP,  PAGE,
		MODEL,  CONTAINER, MAP};

	typedef Metadata			Me;

public:

	Metadata(StringRef name, Byte type): name_(name), typeCode_(type) {}
	virtual ~Metadata() throw () {}

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
				this->GetTypeCode() == Metadata::MAP   ||
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



}}

#endif
