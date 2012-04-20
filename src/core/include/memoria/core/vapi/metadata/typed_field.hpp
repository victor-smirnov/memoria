
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_TYPED_FIELD_HPP
#define _MEMORIA_CORE_API_METADATA_TYPED_FIELD_HPP

#include <memoria/metadata/typed.hpp>
#include <memoria/core/vapi/metadata/number.hpp>



namespace memoria { namespace vapi {



template <typename FieldType, typename Interface>
class TypedFieldImplT: public NumberFieldImplT<Interface> {

	typedef TypedFieldImplT<FieldType, Interface> 	Me;
	typedef NumberFieldImplT<Interface> 			Base;

    typedef Pair<Byte,      TypeCode<Base::BYTE> >            item1;
    typedef Pair<UByte,     TypeCode<Base::UBYTE> >           item2;
    typedef Pair<Short,     TypeCode<Base::SHORT> >           item3;
    typedef Pair<UShort,    TypeCode<Base::USHORT> >          item4;
    typedef Pair<Int,       TypeCode<Base::INT> >             item5;
    typedef Pair<UInt,      TypeCode<Base::UINT> >            item6;
    typedef Pair<BigInt,    TypeCode<Base::BIGINT> >          item7;

    typedef typename TLTool<
            item1,item2,item3,item4,item5,item6,
            item7>::List                   Type2CodeMap;

    static const Int kCode = Type2TypeMap<FieldType, Type2CodeMap, NullType >::Result::Value;

public:

    TypedFieldImplT(Int ptr, Int abi_ptr, StringRef name, Int offset, Int count):
        Base(ptr, abi_ptr, kCode, name, offset, sizeof(FieldType), count) {}

    TypedFieldImplT(Int ptr, Int abi_ptr, StringRef name, Int count):
        Base(ptr, abi_ptr, kCode, name, 0, sizeof(FieldType), count) {}

    TypedFieldImplT(Int ptr, Int abi_ptr, StringRef name):
        Base(ptr, abi_ptr, kCode, name, 0, sizeof(FieldType), 1) {}


    virtual const FieldType& GetValue(const void *mem, Int idx) const {
    	return (const FieldType&) *(const FieldType*) Base::ValuePtr(mem, idx);
    }

    virtual const FieldType &GetValueAbi(const void *mem, Int idx) const {
    	return (const FieldType&)* (const FieldType*) Base::ValuePtrAbi(mem, idx);
    }

    virtual const FieldType &GetValue(const void *mem) const {
    	return GetValue(mem, 0);
    }

    virtual const FieldType &GetValueAbi(const void *mem) const {
    	return GetValueAbi(mem, 0);
    }

    virtual void SetValue(void *mem, const FieldType& v, Int idx) const {
    	FieldMetadataImplT<Interface>::SetValue(mem, &v, idx);
    }

    virtual void SetValueAbi(void *mem, const FieldType& v, Int idx) const {
    	Base::SetValueAbi(mem, &v, idx);
    }

    virtual void SetValue(void *mem, const FieldType& v) const {
    	SetValue(mem, v, 0);
    }

    virtual void SetValueAbi(void *mem, const FieldType& v) const {
    	SetValueAbi(mem, v, 0);
    }

    virtual BigInt GetValue(Page* page, Int idx, bool abi) const{
    	if (abi) {
    		return GetValueAbi(page->Ptr(), idx);
    	}
    	else {
    		return GetValue(page->Ptr(), idx);
    	}
    }

    virtual void SetValue(Page* page, Int idx, BigInt number, bool abi) const {
    	if (abi) {
    		SetValueAbi(page->Ptr(), (FieldType)number, idx);
    	}
    	else {
    		SetValue(page->Ptr(), (FieldType)number, idx);
    	}
    }
};

template <typename FieldType>
class TypedFieldImpl: public TypedFieldImplT<FieldType, TypedField<FieldType> > {
	typedef TypedFieldImplT<FieldType, TypedField<FieldType> > Base;
public:
	TypedFieldImpl(Int ptr, Int abi_ptr, StringRef name):
	        Base(ptr, abi_ptr, name) {}

	TypedFieldImpl(Int ptr, Int abi_ptr, StringRef name, Int count):
		        Base(ptr, abi_ptr, name, count) {}
};


}}


#endif
