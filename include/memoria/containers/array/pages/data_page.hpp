
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
    Short size_;
    Byte value_[Size - sizeof(Size)];

public:
    DynVectorData() {}

    static Int max_size() {
        return Size;
    }

    Int reindex() {return 0;}

    const Short &size() const {
        return size_;
    }

    Short &size() {
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

    void shift(BigInt pos, BigInt length) {
    	MoveBuffer(value_, pos, pos + length, size_ - pos);
    }

    MetadataList GetFields(Long &abi_ptr) const
    {
        MetadataList list;
        FieldFactory<Short>::create(list, size_,  "SIZE", abi_ptr);
        FieldFactory<Byte>::create(list, value_[0], "VALUE", Size, abi_ptr);
        return list;
    }

    void init() {
    	size_ = 0;
    }
};

#pragma pack()

}}

#endif
