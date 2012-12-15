
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
#include <ostream>

#include <malloc.h>

namespace memoria    {
namespace vapi       {

void Expand(std::ostream& os, Int level);

template <typename T>
struct IData {

    virtual ~IData() throw () {}

    virtual SizeT getSize() const                                   = 0;
    virtual void setSize(SizeT size)                                = 0;
    virtual SizeT put(const T* buffer, SizeT start, SizeT length)   = 0;
    virtual SizeT get(T* buffer, SizeT start, SizeT length) const   = 0;
};

template <typename T>
class DataProxy: IData<T> {
    IData<T>&   data_;
    SizeT       start_;
    SizeT       length_;
public:

    DataProxy(IData<T>& data, SizeT start, SizeT length): data_(data), start_(start), length_(length) {}

    virtual ~DataProxy() throw () {}

    virtual SizeT getSize() const
    {
        return length_;
    }

    virtual void setSize(SizeT size) {
        length_ = size;
    }

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        return data_.put(buffer, start + start_, length);
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length) const
    {
        return data_.get(buffer, start + start_, length);
    }
};

template <typename T>
class GetDataProxy: public IData<T> {
    const IData<T>&     data_;
    SizeT               start_;
    SizeT               length_;
public:

    GetDataProxy(const IData<T>& data, SizeT start, SizeT length): data_(data), start_(start), length_(length) {}

    virtual ~GetDataProxy() throw () {}

    virtual SizeT getSize() const
    {
        return length_;
    }

    virtual void setSize(SizeT size) {}

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        return 0;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length) const
    {
        return data_.get(buffer, start + start_, length);
    }
};

template <typename T>
class ArrayData: public IData<T> {
    SizeT   length_;
    T*      data_;
    bool    owner_;
public:

    ArrayData(SizeT length, void* data, bool owner = false):length_(length), data_(T2T<T*>(data)), owner_(owner) {}
    ArrayData(SizeT length):length_(length), data_(T2T<T*>(::malloc(length*sizeof(T)))), owner_(true) {}

    ArrayData(ArrayData<T>&& other):length_(other.length_), data_(other.data_), owner_(other.owner_)
    {
        other.data_ = NULL;
    }

    ArrayData(const ArrayData<T>& other, bool clone = true):length_(other.length_), owner_(true)
    {
        data_ = (T*) ::malloc(length_*sizeof(T));

        if (clone)
        {
            CopyBuffer(other.data(), data_, length_);
        }
    }

    ~ArrayData() throw ()
    {
        if (owner_) ::free(data_);
    }

    SizeT size() const {
        return length_;
    }

    const T* data() const {
        return data_;
    }

    T* data()
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

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        CopyBuffer(buffer, data_ + start, length);
        return length;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length) const
    {
        CopyBuffer(data_ + start, buffer, length);
        return length;
    }

    static ArrayData<T> var(T& ref)
    {
        return ArrayData<T>(sizeof(ref), &ref, false);
    }


    void dump(std::ostream& out) {
        out<<endl;
        Expand(out, 24);
        for (int c = 0; c < 32; c++)
        {
            out.width(3);
            out<<hex<<c;
        }
        out<<endl;

        Int size0 = size();

        for (Int c = 0; c < size0; c+= 32)
        {
            Expand(out, 12);
            out<<" ";
            out.width(4);
            out<<dec<<c<<" "<<hex;
            out.width(4);
            out<<c<<": ";

            for (Int d = 0; d < 32 && c + d < size0; d++)
            {
                UByte data0 = *(this->data() + c + d);
                out<<hex;
                out.width(3);
                out<<(Int)data0;
            }

            out<<dec<<endl;
        }

    }
};


}
}



#endif
