
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_BITMAP_HPP
#define _MEMORIA_CORE_API_METADATA_BITMAP_HPP

#include <memoria/metadata/bitmap.hpp>
#include <memoria/core/vapi/metadata/field.hpp>



namespace memoria { namespace vapi {

template <typename Interface>
class BitmapFieldImplT: public FieldMetadataImplT<Interface> {
	typedef BitmapFieldImplT<Interface> 	Me;
	typedef FieldMetadataImplT<Interface> 	Base;
public:
	BitmapFieldImplT(Int ptr, Int abi_ptr, const string &name, Int offset, Int count, Byte kCode = FieldMetadata::BITMAP): Base(ptr, abi_ptr, kCode, name, offset, offset + count, 1) {}

	virtual BigInt GetBits(Page* page, int idx, int nbits, bool abi) const {
		if (abi) {
			return Base::GetBitsAbi(page->Ptr(), idx, nbits);
		} else {
			return Base::GetBits(page->Ptr(), idx, nbits);
		}
	}

	virtual void SetBits(Page* page, BigInt bits, int idx, int nbits, bool abi) {
		if (abi) {
			Base::SetBitsAbi(page->Ptr(), bits, idx, nbits);
		} else {
			Base::SetBits(page->Ptr(), bits, idx, nbits);
		}
	}




};




typedef BitmapFieldImplT<BitmapField> 						BitmapFieldImpl;



}}


#endif
