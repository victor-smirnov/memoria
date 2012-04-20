
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_TYPED_ID_HPP
#define _MEMORIA_CORE_API_METADATA_TYPED_ID_HPP

#include <memoria/metadata/id.hpp>
#include <memoria/core/vapi/metadata/id.hpp>



namespace memoria { namespace vapi {

template <typename IDType, typename Interface>
class TypedIDFieldImplT: public IDFieldImplT<Interface> {
	typedef TypedIDFieldImplT<IDType, Interface>	Me;
	typedef IDFieldImplT<Interface> 				Base;
public:

	TypedIDFieldImplT(Int ptr, Int abi_ptr, StringRef name, Int offset, Int count):
		Base(ptr, abi_ptr, name, sizeof(IDType), offset, count) {}

	TypedIDFieldImplT(Int ptr, Int abi_ptr, StringRef name, Int count):
		Base(ptr, abi_ptr, name, sizeof(IDType), count) {}

	TypedIDFieldImplT(Int ptr, Int abi_ptr, StringRef name):
		Base(ptr, abi_ptr, name, sizeof(IDType), 1) {}


	virtual const IDValue GetIDValue(const void *mem) const {
		if (mem == NULL) {
			throw NullPointerException("Pointer to ID can't be NULL");
		}

		const IDType* id = static_cast<const IDType*>(mem);

		return IDValue(id);
	}

	virtual void SetIDData(const IDValue& idValue, void* mem) const {
		if (mem == NULL) {
			throw NullPointerException("Pointer to ID can't be NULL");
		}

		IDType* id = static_cast<IDType*>(mem);

		id->CopyFrom(idValue.ptr());
	}


};



template <typename IDType>
class TypedIDFieldImpl: public TypedIDFieldImplT<IDType, IDField> {
	typedef TypedIDFieldImplT<IDType, IDField> Base;

public:
	TypedIDFieldImpl(Int ptr, Int abi_ptr, StringRef name):
			Base(ptr, abi_ptr, name) {}

};



}}


#endif
