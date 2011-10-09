
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_ID_HPP
#define _MEMORIA_VAPI_METADATA_ID_HPP

#include <memoria/metadata/field.hpp>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API IDField: public FieldMetadata {
    virtual void GetValue(Page* page, Int idx, IDValue& value, bool abi) const 	= 0;
    virtual void SetValue(Page* page, Int idx, const IDValue& value, bool abi) const		= 0;

    virtual const IDValue GetIDValue(const void *mem) const							= 0;
    virtual void SetIDData(const IDValue& id, void* mem) const 						= 0;

    
};



//template <typename IDType>
//struct MEMORIA_API TypedIDField: public IDField {
//    virtual const IDValue GetIDValue(const void *mem)  			= 0;
//    virtual void SetIDData(const IDValue& idValue, void* mem)  	= 0;
//};

//MEMORIA_DECLARE_PARAMETRIZED_TYPENAME(TypedIDField, "memoria::vapi::metadata::TypedIDField");

}}

#endif
