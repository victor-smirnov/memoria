
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_ID_HPP
#define _MEMORIA_CORE_API_METADATA_ID_HPP

#include <memoria/metadata/id.hpp>
#include <memoria/core/vapi/metadata/field.hpp>



namespace memoria { namespace vapi {


template <typename Interface>
class IDFieldImplT: public FieldMetadataImplT<Interface> {
	typedef IDFieldImplT<Interface> 		Me;
	typedef FieldMetadataImplT<Interface> 	Base;
public:

	IDFieldImplT(Int ptr, Int abi_ptr, const string &name, Int size, Int offset, Int count):
		Base(ptr, abi_ptr, FieldMetadata::ID, name, offset, offset + count*size, size) {}

	IDFieldImplT(Int ptr, Int abi_ptr, const string &name, Int size, Int count):
		Base(ptr, abi_ptr, FieldMetadata::ID, name, 0, count*size, size) {}

	IDFieldImplT(Int ptr, Int abi_ptr, const string &name, Int size):
		Base(ptr, abi_ptr, FieldMetadata::ID, name, 0, size, size) {}


    virtual void GetValue(Page *page, Int idx, IDValue& value, bool abi) const;
    virtual void SetValue(Page* page, Int idx, const IDValue& value, bool abi) const;

    virtual const IDValue GetIDValue(const void *mem) const = 0;
    virtual void SetIDData(const IDValue& id, void* mem) const = 0;

};




typedef IDFieldImplT<IDField> 							IDFieldImpl;


template <typename Interface>
void IDFieldImplT<Interface>::GetValue(Page *page, Int idx, IDValue& value, bool abi) const
{
    if (page == NULL) {
        throw NullPointerException(MEMORIA_SOURCE, "page must not be NULL");
    }

    void* ptr0 = abi ? Base::ValuePtrAbi(page->Ptr(), idx) : Base::ValuePtr(page->Ptr(), idx);

    value = this->GetIDValue(ptr0);
}

template <typename Interface>
void IDFieldImplT<Interface>::SetValue(Page* page, Int idx, const IDValue& value, bool abi) const
{
    if (page == NULL) {
        throw NullPointerException(MEMORIA_SOURCE, "page must not be NULL");
    }

    void* ptr0 = abi ? Base::ValuePtrAbi(page->Ptr(), idx) : Base::ValuePtr(page->Ptr(), idx);

    this->SetIDData(value, ptr0);
}

}}


#endif
