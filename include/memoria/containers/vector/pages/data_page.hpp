
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_PAGES_DATA_PAGE_HPP
#define	_MEMORIA_MODELS_ARRAY_PAGES_DATA_PAGE_HPP

#include <memoria/core/tools/reflection.hpp>

namespace memoria    {
namespace array      {

#pragma pack(1)

template <Int Size>
class DynVectorData
{
    Int size_;

    Byte value_[Size - sizeof(size_)];

public:
    DynVectorData() {}

    static Int max_size() {
        return Size - sizeof(size_);
    }

    Int reindex() {return 0;}

    const Int &size() const {
        return size_;
    }

    Int &size() {
        return size_;
    }

    const Byte& value(Int idx) const {
        return value_[idx];
    }

    Byte& value(Int idx) {
        return value_[idx];
    }

    Byte* value_addr(Int idx) {
    	return &value_[idx];
    }

    const Byte* value_addr(Int idx) const {
    	return &value_[idx];
    }

    Int byte_size() const {
        return size_ + sizeof(size_);
    }

    void shift(BigInt pos, BigInt length)
    {
    	//FIXME: implement left shift properly
    	if (pos < size_)
    	{
    		MoveBuffer(value_, pos, pos + length, size_ - pos);
    	}
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
    	handler->StartGroup("DATA");

    	handler->Value("SIZE", &size_);
    	handler->Value("VALUE", value_, size_, IPageDataEventHandler::BYTE_ARRAY);

    	handler->EndGroup();
    }

    //template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
    	FieldFactory<Int>::serialize(buf, size_);
    	FieldFactory<Byte>::serialize(buf, value_[0], sizeof(value_));
    }

    //template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
    	FieldFactory<Int>::deserialize(buf, size_);
    	FieldFactory<Byte>::deserialize(buf, value_[0], sizeof(value_));
    }

    void init() {
    	size_ = 0;
    }
};

#pragma pack()

}}

#endif
