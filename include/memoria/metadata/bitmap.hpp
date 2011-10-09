
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_BITMAP_HPP
#define _MEMORIA_VAPI_METADATA_BITMAP_HPP

#include <memoria/metadata/field.hpp>

namespace memoria   {
namespace vapi       {

struct MEMORIA_API BitmapField: public FieldMetadata {

	virtual BigInt GetBits(Page* page, int idx, int nbits, bool abi) const				= 0;
	virtual void   SetBits(Page* page, BigInt bits, int idx, int nbits, bool abi) 		= 0;

    
};



}}

#endif
