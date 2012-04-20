
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_FIELD_HPP
#define _MEMORIA_VAPI_METADATA_FIELD_HPP

#include <memoria/metadata/metadata.hpp>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API FieldMetadata: public Metadata {

    typedef TL<Pair<TypeCode<BYTE>,    Byte>,
            TL<Pair<TypeCode<UBYTE>,   UByte>,
            TL<Pair<TypeCode<SHORT>,   Short>,
            TL<Pair<TypeCode<USHORT>,  UShort>,
            TL<Pair<TypeCode<INT>,     Int>,
            TL<Pair<TypeCode<UINT>,    UInt>,
            TL<Pair<TypeCode<BIGINT>,  BigInt>,
            TL<Pair<TypeCode<ID>,      IDValue> > > > > > > > >       Code2TypeMap;

    virtual Int Ptr() const  												= 0;
    virtual Int AbiPtr() const  											= 0;
    virtual Int Count() const 												= 0;

    virtual Int Size() const 												= 0;
    virtual Int Offset() const  											= 0;
    virtual Int Limit() const 												= 0;

    virtual const void *ValuePtr(const void *mem, Int idx) const 			= 0;
    virtual void *ValuePtr(void *mem, Int idx) const						= 0;

    virtual const void *ValuePtrAbi(const void *mem, Int idx) const 		= 0;
    virtual void *ValuePtrAbi(void *mem, Int idx) const						= 0;


    virtual const void *BitmapPtr(const void *mem) const 					= 0;
    virtual void *BitmapPtr(void *mem) const								= 0;

    virtual const void *BitmapPtrAbi(const void *mem) const 				= 0;
    virtual void *BitmapPtrAbi(void *mem) const								= 0;

    virtual void GetValue(const void *mem, void *value, Int idx) const 		= 0;
    virtual void GetValueAbi(const void *mem, void *value, Int idx) const 	= 0;

    virtual void SetValue(void *mem, const void *value, Int idx) const		= 0;
    virtual void SetValueAbi(void *mem, const void *value, Int idx) const	= 0;

    virtual BigInt GetBits(const void *mem, Int idx, Int nbits) const 		= 0;
    virtual BigInt GetBitsAbi(const void *mem, Int idx, Int nbits) const 	= 0;

    virtual void SetBits(void *mem, BigInt bits, Int idx, Int nbits) const		= 0;
    virtual void SetBitsAbi(void *mem, BigInt bits, Int idx, Int nbits) const	= 0;

    virtual void* CreateValueHolder()										= 0;
    
    virtual void Configure(Int ptr, Int abi_ptr)							= 0;

};



}}

#endif
