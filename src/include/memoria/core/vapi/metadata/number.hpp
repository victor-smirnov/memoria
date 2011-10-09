
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CORE_API_METADATA_NUMBER_HPP
#define _MEMORIA_CORE_API_METADATA_NUMBER_HPP

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/metadata/number.hpp>
#include <memoria/core/vapi/metadata/field.hpp>



namespace memoria { namespace vapi {

template <typename Interface>
class NumberFieldImplT: public FieldMetadataImplT<Interface> {
	typedef NumberFieldImplT<Interface> 		Me;
	typedef FieldMetadataImplT<Interface> 		Base;
public:

	NumberFieldImplT(Int ptr, Int abi_ptr, Byte kCode, StringRef name, Int offset, Int item_size, Int count):
		Base(ptr, abi_ptr, kCode, name, offset, offset + count*item_size, item_size) {}

	NumberFieldImplT(Int ptr, Int abi_ptr, Byte kCode, StringRef name, Int item_size, Int count):
		Base(ptr, abi_ptr, kCode, name, 0, count*item_size, item_size) {}

	virtual BigInt GetValue(Page* page, Int idx, bool abi) const				= 0;
	virtual void SetValue(Page* page, Int idx, BigInt number, bool abi) const 	= 0;

};

typedef NumberFieldImplT<NumberField> 						NumberFieldImpl;







}}


#endif
