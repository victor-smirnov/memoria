
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



namespace memoria { namespace vapi {


template <typename Interface>
class PageMetadataImplT: public MetadataGroupImplT<Interface> {
	typedef PageMetadataImplT<Interface> 	Me;
	typedef MetadataGroupImplT<Interface> 	Base;

public:

    PageMetadataImplT(StringRef name, const MetadataList &content, Int attributes, Int hash0, PageSizeProviderFn page_size_provider, Int page_size);
    virtual ~PageMetadataImplT() throw () {}

    virtual Int Hash() const {
        return hash_;
    }

    virtual bool IsAbiCompatible() const {
        return abi_compatible_;
    }

    virtual Int GetPageSize() const {
    	return page_size_;
    }

    virtual void Externalize(const void *mem, void *buf) const;

    virtual void Internalize(const void *buf, void *mem, Int size) const;

    virtual Int GetDataBlockSize(const void* buf) const {
    	return (*page_size_provider_)(buf);
    }

    virtual Int GetPageDataBlockSize(const Page* tpage) const {
    	const Page* page = static_cast<const Page*>(tpage);

    	if (page == NULL) {
    		throw NullPointerException(MEMORIA_SOURCE, "Page must not be null");
    	}

    	return (*page_size_provider_)(page->Ptr());
    }

    virtual const FieldMetadata* GetField(Int ptr, bool abi) const;

    virtual FieldMetadata* GetLastField() const {
    	return last_field_;
    }

private:
    Int  hash_;
    bool abi_compatible_;
    bool attributes_;
    Int page_size_;
    PageSizeProviderFn page_size_provider_;
    PtrMap ptr_map_;
    PtrMap abi_map_;

    FieldMetadata* last_field_;
};

typedef PageMetadataImplT<PageMetadata> 					PageMetadataImpl;



class ComputeHashFn {
    Int hash_;
    bool abi_compatible_;
public:

    ComputeHashFn(Int hash, bool abi_compatible) : hash_(hash), abi_compatible_(abi_compatible) {
    }

    void operator()(FieldMetadata *field) {
        if (field->GetTypeCode() != Metadata::BITMAP) {
            hash_ = (field->AbiPtr() * 1211) ^ CShr(hash_, 3) ^ (field->GetTypeCode() + 16 * field->Count() + field->Offset() * 256 + field->Limit() * 65536);
        }

        if (abi_compatible_) {
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

class BuildPtrMapFn {
    PtrMap& ptr_map_;
    PtrMap& abi_map_;
public:

    BuildPtrMapFn(PtrMap& ptr_map, PtrMap& abi_map): ptr_map_(ptr_map), abi_map_(abi_map)  {}

    void operator()(FieldMetadata *field)
    {
        ptr_map_[(Int)field->Ptr()] = field;
        abi_map_[(Int)field->AbiPtr()] = field;
    }
};

template <bool Internalize_>
class InternalizeExternalizeFn {
    const void *src_;
    void *tgt_;

public:

    InternalizeExternalizeFn(const void *src, void *tgt) :
        src_(src), tgt_(tgt) {}

    void operator()(FieldMetadata *field)
    {
        if (Internalize_)
        {
            if (field->GetTypeCode() == Metadata::BITMAP || field->GetTypeCode() == Metadata::FLAG)
            {
                const char* csrc = static_cast<const char*> (field->BitmapPtrAbi(src_));
                char* ctgt = static_cast<char*> (field->BitmapPtr(tgt_));
                memoria::CopyBits(csrc, ctgt, field->Offset(), field->Offset(), field->Count());
            }
            else
            {
                for (Int c = 0; c < field->Count(); c++) {
                    field->SetValue(tgt_, field->ValuePtrAbi(src_, c), c);
                }
            }
        }
        else
        {
            if (field->GetTypeCode() == Metadata::BITMAP || field->GetTypeCode() == Metadata::FLAG)
            {
                const char* csrc = static_cast<const char*> (field->BitmapPtr(src_));
                char* ctgt = static_cast<char*> (field->BitmapPtrAbi(tgt_));

                memoria::CopyBits(csrc, ctgt, field->Offset(), field->Offset(), field->Count());
            }
            else
            {
                for (Int c = 0; c < field->Count(); c++)
                {
                    const void* ptr = field->ValuePtr(src_, c);
                    field->SetValueAbi(tgt_, ptr, c);
                }
            }
        }
    }
};



class FindMaxFieldFn {

	FieldMetadata* field_;

public:

	FindMaxFieldFn() :field_(NULL) {}

	FieldMetadata* field() const {
		return field_;
	}

    void operator()(FieldMetadata *field)
    {
        if (field_ == NULL || field->Ptr() >= field_->Ptr())
        {
        	field_ = field;
        }
    }
};



template <typename Functor>
bool ForAllFields(const MetadataGroup *group, Functor &functor, Int limit) {
    for (Int c = 0; c < group->Size(); c++) {
        if (group->GetItem(c)->GetTypeCode() == Metadata::GROUP) {
            if (ForAllFields(static_cast<const MetadataGroup*> (group->GetItem(c)), functor, limit)) {
                return true;
            }
        }
        else {
            FieldMetadata* field = static_cast<FieldMetadata*> (group->GetItem(c));

            if (field->Ptr() < limit) {
                functor(field);
            }
            else return true; // limit is reached
        }
    }

    return false;
}

template <typename Interface>
PageMetadataImplT<Interface>::PageMetadataImplT(StringRef name, const MetadataList &content, Int attributes, Int hash0, PageSizeProviderFn page_size_provider, Int page_size):
    Base(name, content), attributes_(attributes), last_field_(NULL)
{
    Base::set_type() = Metadata::PAGE;
    abi_compatible_ = true;
    hash_ = hash0;
    page_size_provider_ = page_size_provider;
    page_size_ = page_size;

    if (page_size_provider_ == NULL) {
        throw NullPointerException(MEMORIA_SOURCE, "Page size provider is not specified");
    }

    ComputeHashFn fn(hash_, abi_compatible_);

    ForAllFields(this, fn, page_size);

    hash_ = fn.hash();
    abi_compatible_ = fn.abi_compatible();

    BuildPtrMapFn build_fn(ptr_map_, abi_map_);

    ForAllFields(this, build_fn, page_size);

    FindMaxFieldFn findMax_fn;
    ForAllFields(this, findMax_fn, page_size);
}


template <typename Interface>
void PageMetadataImplT<Interface>::Externalize(const void *mem, void *buf) const {
    Int ptr = GetDataBlockSize(mem);
    InternalizeExternalizeFn < false > fn(mem, buf);
    ForAllFields(this, fn, ptr);
}

template <typename Interface>
void PageMetadataImplT<Interface>::Internalize(const void *buf, void *mem, Int size) const {
    if (size == -1) size = GetPageSize();
    InternalizeExternalizeFn < true > fn(buf, mem);
    ForAllFields(this, fn, size);
}


template <typename Interface>
const FieldMetadata* PageMetadataImplT<Interface>::GetField(Int ptr, bool abi) const
{
    const PtrMap* map = abi ? &abi_map_ : &ptr_map_;
    PtrMap::const_iterator i = map->find(ptr);

    if (i != map->end()) {
        return i->second;
    }
    else {
        //throw MemoriaException(MEMORIA_SOURCE, "Can't find field for the specified field ptr");
        return NULL;
    }
}


}}


#endif
