
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_NUMBER_HPP
#define _MEMORIA_VAPI_METADATA_NUMBER_HPP

#include <memoria/metadata/field.hpp>


namespace memoria    {
namespace vapi       {

struct MEMORIA_API NumberField: public FieldMetadata {
//	virtual void GetValue(Page* page, Int idx, Number* number, bool abi) const 	= 0;
//	virtual void SetValue(Page* page, Int idx, Number* number, bool abi) const 	= 0;

    virtual BigInt GetValue(Page* page, Int idx, bool abi) const				= 0;
    virtual void SetValue(Page* page, Int idx, BigInt number, bool abi) const	= 0;

    
};


}}

#endif
