
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_PAGES_PARTS_HPP
#define	_MEMORIA_MODELS_ARRAY_PAGES_PARTS_HPP

#include <memoria/prototypes/btree/pages/parts.hpp>

namespace memoria    {


template <typename ID>
class VectorMetadata: public BTreeMetadata<ID>
{
	typedef BTreeMetadata<ID> Base;

	Int element_size_;

public:
	VectorMetadata() {}

    void GenerateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::GenerateDataEvents(handler);
    	handler->Value("ELEMENT_SIZE", &element_size_);
    }

    Int& element_size() {
    	return element_size_;
    }

    const Int& element_size() const {
    	return element_size_;
    }

    void Serialize(SerializationData& buf) const
    {
    	Base::Serialize(buf);

    	FieldFactory<Int>::serialize(buf, element_size_);
    }

    void Deserialize(DeserializationData& buf)
    {
    	Base::Deserialize(buf);

    	FieldFactory<Int>::deserialize(buf, element_size_);
    }
};

}

#endif
