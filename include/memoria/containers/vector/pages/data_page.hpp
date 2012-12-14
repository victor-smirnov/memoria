
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_PAGES_DATA_PAGE_HPP
#define _MEMORIA_MODELS_ARRAY_PAGES_DATA_PAGE_HPP

#include <memoria/core/tools/reflection.hpp>

namespace memoria    {
namespace array      {

#pragma pack(1)

template <typename ElementType_>
class DynVectorData
{
    static const UInt VERSION = 1;

    Int size_;

    ElementType_ value_[];

public:
    typedef ElementType_                                                        ElementType;

    typedef TypeList<
        ConstValue<UInt, VERSION>,
        decltype(size_),
        ElementType
    >                                                                           FieldsList;


    DynVectorData() {}

    Int reindex() {return 0;}

    const Int &size() const {
        return size_;
    }

    Int &size() {
        return size_;
    }

    Int data_size() const {
        return size_ * sizeof(ElementType);
    }

    const ElementType& value(Int idx) const {
        return value_[idx];
    }

    ElementType& value(Int idx) {
        return value_[idx];
    }

    ElementType* value_addr(Int idx) {
        return &value_[idx];
    }

    const ElementType* value_addr(Int idx) const {
        return &value_[idx];
    }

    void shift(BigInt pos, BigInt length)
    {
        //FIXME: implement left shift properly
        if (pos < size_)
        {
            MoveBuffer(value_, pos, pos + length, size_ - pos);
        }
    }

    void copyTo(DynVectorData<ElementType_>* target) const
    {
        target->size() = size();
        CopyBuffer(value_, target->value_, size_);
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("DATA");

        handler->value("SIZE", &size_);
        handler->value("VALUE", value_, size_, IPageDataEventHandler::BYTE_ARRAY); // FIXME; use correct data type handler

        handler->endGroup();
    }

    //template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<Int>::serialize(buf, size_);
        FieldFactory<ElementType>::serialize(buf, value_[0], size_);
    }

    //template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<Int>::deserialize(buf, size_);
        FieldFactory<ElementType>::deserialize(buf, value_[0], size_);
    }

    void init() {
        size_ = 0;
    }
};

#pragma pack()

}}

#endif
