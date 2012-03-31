
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_GROUP_HPP
#define _MEMORIA_CORE_API_METADATA_GROUP_HPP

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/metadata/group.hpp>
#include <memoria/core/vapi/metadata/metadata.hpp>



namespace memoria { namespace vapi {

template <typename Interface>
class MetadataGroupImplT: public MetadataImplT<Interface> {
	typedef MetadataGroupImplT<Interface> 		Me;
	typedef MetadataImplT<Interface> 			Base;
public:

	MetadataGroupImplT(StringRef name, const MetadataList &content, Int block_size = 0): Base(name, Metadata::GROUP), content_(content.size()), block_size_(block_size)
	{
		for (UInt c = 0; c < content.size(); c++) {
			content_[c] = content[c];
		}
	}

	virtual ~MetadataGroupImplT() throw ()
	{
		for (UInt c = 0; c < content_.size(); c++)
		{

			if (content_[c]->GetTypeCode() != Metadata::PAGE &&
						content_[c]->GetTypeCode() != Metadata::MODEL &&
						content_[c]->GetTypeCode() != Metadata::CONTAINER)
			{
				delete content_[c];
			}
		}
	}


	virtual Int Size() const {
		return content_.size();
	}

	virtual Metadata* GetItem(Int idx) const {
		return content_[idx];
	}

	virtual Metadata* FindFirst(const char* name, bool throwEx = false)
	{
		for (Int c = 0; c < Size(); c++)
		{
			Metadata* item = GetItem(c);
			if (item->Name() == name)
			{
				return item;
			}
			//FIXME: IsGroup
			else if (item->IsGroup())
			{
				return FindFirst(name, throwEx);
			}
		}

		if (throwEx)
		{
			throw new MemoriaException(MEMORIA_SOURCE, String("Can't find metadata filed: ") + name);
		}
		else {
			return NULL;
		}
	}


	virtual const FieldMetadata* FindFirstField() const
	{
		return FindFirstField(this);
	}

	virtual void PutAll(MetadataList& target) const
	{
		for (auto i = content_.begin(); i != content_.end(); i++)
		{
			target.push_back(*i);
		}
	}

	virtual Int GetBlockSize() const
	{
		return block_size_;
	}

private:
    MetadataList 	content_;
    Int 			block_size_;

    const FieldMetadata* FindFirstField(const MetadataGroup* group) const
    {
    	for (Int c = 0; c < Size(); c++)
    	{
    		const Metadata* item = group->GetItem(c);
    		if (item->IsField())
    		{
    			return T2T<const FieldMetadata*>(item);
    		}
    		else if (item->IsGroup())
    		{
    			return FindFirstField(static_cast<const MetadataGroup*>(item));
    		}
    	}

    	return NULL;
    }
};


typedef MetadataGroupImplT<MetadataGroup> 					MetadataGroupImpl;



}}


#endif
