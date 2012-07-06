
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
public:

	MetadataGroup(StringRef name, const MetadataList &content, Int block_size = 0, Int type = Metadata::GROUP): Metadata(name, type), content_(content.size()), block_size_(block_size)
	{
		for (UInt c = 0; c < content.size(); c++) {
			content_[c] = content[c];
		}
	}

	virtual ~MetadataGroup() throw ()
	{
		for (UInt c = 0; c < content_.size(); c++)
		{

			if (content_[c]->getTypeCode() != Metadata::PAGE &&
					content_[c]->getTypeCode() != Metadata::MODEL &&
					content_[c]->getTypeCode() != Metadata::CONTAINER)
			{
				delete content_[c];
			}
		}
	}


	virtual Int size() const {
		return content_.size();
	}

	virtual Metadata* getItem(Int idx) const {
		return content_[idx];
	}

//	virtual Metadata* findFirst(const char* name, bool throwEx = false)
//	{
//		for (Int c = 0; c < Size(); c++)
//		{
//			Metadata* item = getItem(c);
//			if (item->Name() == name)
//			{
//				return item;
//			}
//			//FIXME: IsGroup
//			else if (item->IsGroup())
//			{
//				return findFirst(name, throwEx);
//			}
//		}
//
//		if (throwEx)
//		{
//			throw new Exception(MEMORIA_SOURCE, String("Can't find metadata filed: ") + name);
//		}
//		else {
//			return NULL;
//		}
//	}
//
//
//	virtual const FieldMetadata* findFirstField() const
//	{
//		return findFirstField(this);
//	}

	virtual void putAll(MetadataList& target) const
	{
		for (auto i = content_.begin(); i != content_.end(); i++)
		{
			target.push_back(*i);
		}
	}

	virtual Int getBlockSize() const
	{
		return block_size_;
	}

protected:
	MetadataList 	content_;
	Int 			block_size_;

//	const FieldMetadata* findFirstField(const MetadataGroup* group) const
//	{
//		for (Int c = 0; c < Size(); c++)
//		{
//			const Metadata* item = group->getItem(c);
//			if (item->IsField())
//			{
//				return T2T<const FieldMetadata*>(item);
//			}
//			else if (item->IsGroup())
//			{
//				return findFirstField(static_cast<const MetadataGroup*>(item));
//			}
//		}
//
//		return NULL;
//	}
};



inline bool isGroup(Metadata *meta)
{
    Int type = meta->getTypeCode();
    return type == Metadata::GROUP || type == Metadata::CONTAINER || type == Metadata::MODEL ||
        type == Metadata::PAGE;
}



}}

#endif
