
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_ARRAY_DATA_HPP
#define _MEMORIA_CORE_TOOLS_ARRAY_DATA_HPP


#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <vector>

#include <malloc.h>

namespace memoria    {
namespace vapi       {

struct IData {

    virtual ~IData() throw () {}

    virtual SizeT getSize() const                                       = 0;
    virtual void setSize(SizeT size)                                    = 0;
    virtual SizeT put(const Byte* buffer, SizeT start, SizeT length)    = 0;
    virtual SizeT get(Byte* buffer, SizeT start, SizeT length) const    = 0;
};

class DataProxy: IData {
    IData&  data_;
    SizeT   start_;
    SizeT   length_;
public:

    DataProxy(IData& data, SizeT start, SizeT length): data_(data), start_(start), length_(length) {}

    virtual ~DataProxy() throw () {}

    virtual SizeT getSize() const
    {
        return length_;
    }

    virtual void setSize(SizeT size) {
        length_ = size;
    }

    virtual SizeT put(const Byte* buffer, SizeT start, SizeT length)
    {
        return data_.put(buffer, start + start_, length);
    }

    virtual SizeT get(Byte* buffer, SizeT start, SizeT length) const
    {
        return data_.get(buffer, start + start_, length);
    }
};


class GetDataProxy: public IData {
    const IData&    data_;
    SizeT           start_;
    SizeT           length_;
public:

    GetDataProxy(const IData& data, SizeT start, SizeT length): data_(data), start_(start), length_(length) {}

    virtual ~GetDataProxy() throw () {}

    virtual SizeT getSize() const
    {
        return length_;
    }

    virtual void setSize(SizeT size) {}

    virtual SizeT put(const Byte* buffer, SizeT start, SizeT length)
    {
        return 0;
    }

    virtual SizeT get(Byte* buffer, SizeT start, SizeT length) const
    {
        return data_.get(buffer, start + start_, length);
    }
};


class ArrayData: public IData {
    Int length_;
    UByte* data_;
    bool owner_;
public:

    template <typename T>
    ArrayData(T& value):
        length_(sizeof(T)),
        data_(T2T<UByte*>(&value)),
        owner_(false)
    {}

    ArrayData(Int length, void* data, bool owner = false):length_(length), data_(T2T<UByte*>(data)), owner_(owner) {}
    ArrayData(Int length):length_(length), data_(T2T<UByte*>(::malloc(length))), owner_(true) {}

    ArrayData(ArrayData&& other):length_(other.length_), data_(other.data_), owner_(other.owner_)
    {
        other.data_ = NULL;
    }

    ArrayData(const ArrayData& other, bool clone = true):length_(other.length_), owner_(true)
    {
        data_ = (UByte*) ::malloc(length_);

        if (clone)
        {
            CopyBuffer(other.data(), data_, length_);
        }
    }

    ~ArrayData() throw () {
        if (owner_) ::free(data_);
    }

    Int size() const {
        return length_;
    }

    const UByte* data() const {
        return data_;
    }

    UByte* data()
    {
        return data_;
    }

    virtual SizeT getSize() const
    {
        return length_;
    }

    virtual void setSize(SizeT size)
    {
        length_ = size;
    }

    virtual SizeT put(const Byte* buffer, SizeT start, SizeT length)
    {
        CopyBuffer(buffer, data_ + start, length);
        return length;
    }

    virtual SizeT get(Byte* buffer, SizeT start, SizeT length) const
    {
        CopyBuffer(data_ + start, buffer, length);
        return 0;
    }


    void dump(std::ostream& out);
};


}
}



#endif
