
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

	MetadataGroupImplT(StringRef name, const MetadataList &content): Base(name, Metadata::GROUP), content_(content.size()) {
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

private:
    MetadataList content_;
};


typedef MetadataGroupImplT<MetadataGroup> 					MetadataGroupImpl;



}}


#endif
