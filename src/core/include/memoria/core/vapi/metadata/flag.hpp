
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_METADATA_FLAG_HPP
#define _MEMORIA_CORE_API_METADATA_FLAG_HPP

#include <memoria/metadata/flag.hpp>
#include <memoria/core/vapi/metadata/bitmap.hpp>



namespace memoria { namespace vapi {

template <typename Interface>
class FlagFieldImplT: public BitmapFieldImplT<Interface> {
	typedef FlagFieldImplT<Interface> 		Me;
	typedef BitmapFieldImplT<Interface> 	Base;
public:
	FlagFieldImplT(Int ptr, Int abi_ptr, const string &name, Int offset, Int count): Base(ptr, abi_ptr, name, offset, offset, FieldMetadata::FLAG) {}
	FlagFieldImplT(Int ptr, Int abi_ptr, const string &name, Int offset): Base(ptr, abi_ptr, name, offset, 1, FieldMetadata::FLAG) {}

};



typedef FlagFieldImplT<FlagField> 						FlagFieldImpl;



}}


#endif
