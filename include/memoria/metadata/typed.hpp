
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_TYPED_HPP
#define _MEMORIA_VAPI_METADATA_TYPED_HPP

#include <memoria/metadata/number.hpp>


namespace memoria    {
namespace vapi       {

template <typename FieldType>
struct MEMORIA_API TypedField: public NumberField {

    virtual const FieldType& GetValue(const void *mem, Int idx) const 				= 0;
    virtual const FieldType& GetValueAbi(const void *mem, Int idx) const  			= 0;

    virtual const FieldType& GetValue(const void *mem) const  						= 0;
    virtual const FieldType& GetValueAbi(const void *mem) const  					= 0;

    virtual void SetValue(void *mem, const FieldType& v, Int idx) const  			= 0;
    virtual void SetValueAbi(void *mem, const FieldType& v, Int idx) const			= 0;

    virtual void SetValue(void *mem, const FieldType& v) const						= 0;
    virtual void SetValueAbi(void *mem, const FieldType& v) const					= 0;

    virtual BigInt GetValue(Page* page, Int idx, bool abi) const 					= 0;
    virtual void SetValue(Page* page, Int idx, BigInt number, bool abi) const		= 0;

    
};

typedef TypedField<Byte> 	ByteField;
typedef TypedField<UByte> 	UByteField;
typedef TypedField<Short> 	ShortField;
typedef TypedField<UShort> 	UShortField;
typedef TypedField<Int> 	IntField;
typedef TypedField<BigInt> 	BigIntField;



}}


#endif
