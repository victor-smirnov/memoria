
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_PAGE_HPP
#define _MEMORIA_CORE_API_METADATA_PAGE_HPP

#include <memoria/metadata/page.hpp>
#include <memoria/core/vapi/metadata/group.hpp>
#include <memoria/core/vapi/metadata/field.hpp>
#include <memoria/core/vapi/metadata/page_wrapper.hpp>

#include <memoria/core/exceptions/memoria.hpp>
#include <memoria/core/tools/strings.hpp>

#include <vector>
#include <strings.h>

#include <iostream>

namespace memoria { namespace vapi {

using namespace std;




template <typename Interface>
class PageMetadataImplT: public MetadataGroupImplT<Interface> {
	typedef PageMetadataImplT<Interface> 	Me;
	typedef MetadataGroupImplT<Interface> 	Base;

public:

    PageMetadataImplT(StringRef name, const MetadataList &content, Int attributes, Int hash0, const IPageOperations* page_operations, Int page_size);
    virtual ~PageMetadataImplT() throw () {}

    virtual Int Hash() const {
        return hash_;
    }


    virtual const IPageOperations* GetPageOperations() const {
    	return page_operations_;
    }

private:
    Int  hash_;
    bool attributes_;
    Int page_size_;

    const IPageOperations* page_operations_;
};

typedef PageMetadataImplT<PageMetadata> 					PageMetadataImpl;



class ComputeHashFn {
    Int hash_;
    bool abi_compatible_;
public:

    ComputeHashFn(Int hash, bool abi_compatible) : hash_(hash), abi_compatible_(abi_compatible) {}

    void operator()(const MetadataGroup *group, FieldMetadata *field)
    {
        if (field->GetTypeCode() != Metadata::BITMAP)
        {
            hash_ = (field->AbiPtr() * 1211) ^ CShr(hash_, 3);// ^ (field->GetTypeCode() + 16 * field->Count() + field->Offset() * 256 + field->Limit() * 65536);
        }

        if (abi_compatible_)
        {
            abi_compatible_ = field->AbiPtr() == field->Ptr();
        }
    }

    Int hash() const {
        return hash_;
    }

    bool abi_compatible() {
        return abi_compatible_;
    }
};





template <typename Functor>
bool ForAllFields(const MetadataGroup *group, Functor &functor, Int limit)
{
    for (Int c = 0; c < group->Size(); c++)
    {
        if (group->GetItem(c)->GetTypeCode() == Metadata::GROUP || group->GetItem(c)->GetTypeCode() == Metadata::MAP)
        {
            if (ForAllFields(static_cast<const MetadataGroup*> (group->GetItem(c)), functor, limit))
            {
                return true;
            }
        }
        else {
            FieldMetadata* field = static_cast<FieldMetadata*> (group->GetItem(c));

            if (field->Ptr() < limit)
            {
                functor(group, field);
            }
            else return true; // limit is reached
        }
    }

    return false;
}


template <typename Functor>
bool ForAllFieldsAndFiledGroups(const MetadataGroup *group, Functor &functor, Int limit)
{
	if (group->GetBlockSize() > 0)
	{
		const FieldMetadata* field = group->FindFirstField();
		if (field != NULL)
		{
			functor(group);
			return field->Ptr() + group->GetBlockSize() >= limit;
		}
		else {
			// Empty group. This shouldn't happen
			return false;
		}
	}
	else {
		for (Int c = 0; c < group->Size(); c++)
		{
			if (group->GetItem(c)->GetTypeCode() == Metadata::GROUP)
			{
				const MetadataGroup* child_group = static_cast<const MetadataGroup*>((group->GetItem(c)));
				if (ForAllFieldsAndFiledGroups(child_group, functor, limit))
				{
					return true;
				}
			}
			else {
				FieldMetadata* field = static_cast<FieldMetadata*> (group->GetItem(c));

				if (field->Ptr() < limit)
				{
					functor(group, field);
				}
				else return true; // limit is reached
			}
		}
	}

    return false;
}


template <typename Interface>
PageMetadataImplT<Interface>::PageMetadataImplT(StringRef name, const MetadataList &content, Int attributes, Int hash0, const IPageOperations* page_operations, Int page_size):
    Base(name, content), attributes_(attributes)
{
	Base::set_type() = Metadata::PAGE;
    hash_ = hash0;
    page_operations_ = page_operations;
    page_size_ = page_size;

    if (page_operations == NULL)
    {
        throw NullPointerException(MEMORIA_SOURCE, "Page size provider is not specified");
    }

    bool abi_compatible = true;
    ComputeHashFn fn(hash_, abi_compatible);

    ForAllFields(this, fn, page_size);

    hash_ = fn.hash();
}

}}


#endif
