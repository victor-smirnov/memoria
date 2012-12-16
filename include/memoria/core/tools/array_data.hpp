
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
#include <vector>

#include <malloc.h>

namespace memoria    {
namespace vapi       {

using namespace std;

void Expand(std::ostream& os, Int level);

namespace internal {

template <typename T>
T cvt(T value) {
	return value;
}


inline UByte cvt(Byte value) {
	return (Byte)value;
}

}


template <typename T>
void dumpArray(std::ostream& out_, const T* data, Int count)
{
	Int columns;

	switch (sizeof(T)) {
	case 1: columns = 32; break;
	case 2: columns = 16; break;
	case 4: columns = 16; break;
	default: columns = 8;
	}

	Int width = sizeof(T) * 2 + 1;

	out_<<endl;
	Expand(out_, 19 + width);
	for (int c = 0; c < columns; c++)
	{
		out_.width(width);
		out_<<hex<<c;
	}
	out_<<endl;

	for (Int c = 0; c < count; c+= columns)
	{
		Expand(out_, 12);
		out_<<" ";
		out_.width(6);
		out_<<dec<<c<<" "<<hex;
		out_.width(6);
		out_<<c<<": ";

		for (Int d = 0; d < columns && c + d < count; d++)
		{
			out_<<hex;
			out_.width(width);
			out_<<internal::cvt(data[c + d]);
		}

		out_<<dec<<endl;
	}
}




template <typename T>
struct IData {

    virtual ~IData() throw () {}

    virtual SizeT getSize() const                                   = 0;
    virtual void  setSize(SizeT size)                               = 0;
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
class MemBuffer: public IData<T> {
protected:
    SizeT   length_;
    T*      data_;
    bool    owner_;
public:

    MemBuffer(T* data, SizeT length, bool owner = false):
    	length_(length),
    	data_(data),
    	owner_(owner)
    {}

    MemBuffer(SizeT length):
    	length_(length),
    	data_(T2T<T*>(::malloc(length * sizeof(T)))),
    	owner_(true)
    {}

    MemBuffer(MemBuffer<T>&& other):
    	length_(other.length_),
    	data_(other.data_),
    	owner_(other.owner_)
    {
        other.data_ = NULL;
    }

    MemBuffer(const MemBuffer<T>& other):
    	length_(other.length_),
    	owner_(true)
    {
        data_ = T2T<T*>(::malloc(length_*sizeof(T)));

        CopyBuffer(other.data(), data_, length_);
    }

    virtual ~MemBuffer() throw ()
    {
        if (owner_) ::free(data_);
    }

    virtual SizeT getSize() const
    {
        return length_;
    }

    T* data()
    {
    	return data_;
    }

    const T* data() const
    {
    	return data_;
    }

    SizeT size() const {
    	return getSize();
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

    void dump(std::ostream& out) const {
    	dumpArray(out, data_, length_);
    }
};


template <typename T>
class MemBuffer<const T>: public IData<T> {
protected:
    SizeT   	length_;
    const T*    data_;
public:

    MemBuffer(const T* data, SizeT length):
    	length_(length),
    	data_(data)
    {}

    MemBuffer(const MemBuffer<const T>& other):
    	data_(other.data_),
    	length_(other.length_)
    {}

    virtual ~MemBuffer() throw () {}

    virtual SizeT getSize() const
    {
        return length_;
    }

    const T* data() const
    {
    	return data_;
    }

    SizeT size() const {
    	return getSize();
    }

    virtual void setSize(SizeT size)
    {
    	length_ = size;
    }

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
    	return 0;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length) const
    {
    	CopyBuffer(data_ + start, buffer, length);
    	return length;
    }

    void dump(std::ostream& out) const {
    	dumpArray(out, data_, length_);
    }
};



template <typename T>
class VariableRef: public IData<T> {
protected:
    T&      value_;
public:
    VariableRef(T& value): value_(value) {}

    virtual ~VariableRef() throw () {}

    virtual SizeT getSize() const
    {
    	return 1;
    }

    virtual void setSize(SizeT size) {}


    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
    	value_ = *buffer;
    	return 1;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length) const
    {
    	*buffer = value_;
    	return 1;
    }

    operator T() const {
    	return value_;
    }

    operator const T&() const {
    	return value_;
    }

    operator T&() {
    	return value_;
    }
};

template <typename T>
class VariableRef<const T>: public IData<T> {
protected:
    const T&      value_;
public:
    VariableRef(const T& value): value_(value) {}

    virtual ~VariableRef() throw () {}

    virtual SizeT getSize() const
    {
    	return 1;
    }

    virtual void setSize(SizeT size) {}


    virtual SizeT put(const T* buffer, SizeT start, SizeT length) {
    	return 0;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length) const
    {
    	*buffer = value_;
    	return 1;
    }

    operator T() const {
    	return value_;
    }

    operator const T&() const {
    	return value_;
    }
};


template <typename T>
class VariableValue: public IData<T> {
protected:
    T value_;
public:
    VariableValue(): value_() {}

    VariableValue(const T& value): value_(value) {}

    virtual ~VariableValue() throw () {}

    virtual SizeT getSize() const
    {
    	return 1;
    }

    virtual void setSize(SizeT size) {}

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
    	value_ = *buffer;
    	return 1;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length) const
    {
    	*buffer = value_;
    	return 1;
    }

    operator T() const {
    	return value_;
    }

    operator const T&() const {
    	return value_;
    }

    operator T&() {
    	return value_;
    }
};

}
}



#endif
