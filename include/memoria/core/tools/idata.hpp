
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_IDATA_HPP
#define _MEMORIA_CORE_TOOLS_IDATA_HPP


#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <vector>
#include <iostream>
#include <vector>

#include <malloc.h>

namespace memoria    {
namespace vapi       {

using namespace std;

void Expand(std::ostream& os, Int level);

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
            if (sizeof(T) == 1)
            {
                out_<<(Int)(UByte)data[c + d];
            }
            else {
                out_<<data[c + d];
            }
        }

        out_<<dec<<endl;
    }
}

template <typename T>
void dumpSymbols(ostream& out_, T* symbols, Int size_, Int bits_per_symbol)
{
	Int columns;

	switch (bits_per_symbol)
	{
		case 1: columns = 100; break;
		case 2: columns = 100; break;
		case 4: columns = 100; break;
		default: columns = 50;
	}

	Int width = bits_per_symbol <= 4 ? 1 : 3;

	Int c = 0;

	do
	{
		out_<<endl;
		Expand(out_, 31 - width * 5 - (bits_per_symbol <= 4 ? 2 : 0));
		for (int c = 0; c < columns; c += 5)
		{
			out_.width(width*5);
			out_<<dec<<c;
		}
		out_<<endl;

		Int rows = 0;
		for (; c < size_ && rows < 10; c += columns, rows++)
		{
			Expand(out_, 12);
			out_<<" ";
			out_.width(6);
			out_<<dec<<c<<" "<<hex;
			out_.width(6);
			out_<<c<<": ";

			for (Int d = 0; d < columns && c + d < size_; d++)
			{
				out_<<hex;
				out_.width(width);
				out_<<GetBits(symbols, c + d, bits_per_symbol);
			}

			out_<<dec<<endl;
		}
	} while (c < size_);
}

struct IData {};

struct INodeLayoutManager {
	virtual Int getNodeCapacity(const Int* sizes, Int stream) 		= 0;
};

struct ISource {
	virtual Int streams()											= 0;
	virtual IData* stream(Int stream) 								= 0;

	virtual void newNode(INodeLayoutManager* layout_manager, BigInt* sizes)	= 0;
	virtual BigInt getTotalNodes(INodeLayoutManager* layout_manager)= 0;
};

struct ITarget {
	virtual Int streams()											= 0;
	virtual IData* stream(Int stream) 								= 0;
};




struct IDataBase: IData {

    virtual ~IDataBase() throw () {}

    virtual SizeT skip(SizeT length)                                = 0;
    virtual SizeT getStart() const                                  = 0;
    virtual SizeT getRemainder() const                              = 0;
    virtual SizeT getSize() const                                   = 0;
    virtual void  reset()                                           = 0;
};



template <typename T>
struct IDataSource: IDataBase {

    virtual ~IDataSource() throw () {}

    virtual SizeT get(T* buffer, SizeT start, SizeT length)               		= 0;
};


template <typename T>
struct IDataTarget: IDataBase {

    virtual ~IDataTarget() throw () {}

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)               = 0;
};


template <typename T>
class DataSourceProxy: IDataSource<T> {
	typedef IDataSource<T> 					Base;
	typedef Base 							Delegate;

	Delegate& 	data_;
	BigInt 		size_;
public:
	DataSourceProxy(Delegate& data, SizeT size):
		data_(data),
		size_(size)
	{
		MEMORIA_ASSERT_TRUE(data.getSize() >= size);
	}

    virtual SizeT skip(SizeT length)
    {
    	return data_.skip(length);
    }

    virtual SizeT getStart() const
    {
    	return data_.getStart();
    }

    virtual SizeT getRemainder() const
    {
    	return size_ - data_.getStart();
    }

    virtual SizeT getSize() const
    {
    	return size_;
    }

    virtual void reset()
    {
    	data_.reset();
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
    	return data_.get(buffer, start, length);
    }
};


//template <typename T>
//struct IDataST: IDataSource<T>, IDataTarget<T> {
//    virtual ~IDataST() throw () {}
//};



//template <typename T>
//class DataProxy: public IDataST<T> {
//    IDataST<T>&   data_;
//    SizeT       length_;
//public:
//
//    DataProxy(IDataST<T>& data, SizeT length): data_(data), length_(length) {}
//
//    virtual ~DataProxy() throw () {}
//
//    virtual SizeT getSize() const
//    {
//        return length_;
//    }
//
//    virtual SizeT skip(SizeT length)
//    {
//        return data_.skip(length);
//    }
//
//    virtual SizeT getStart() const
//    {
//        return data_.getStart();
//    }
//
//    virtual SizeT getRemainder() const
//    {
//        return length_ - getStart();
//    }
//
//    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
//    {
//        return data_.put(buffer, start, length);
//    }
//
//    virtual SizeT get(T* buffer, SizeT start, SizeT length)
//    {
//        return data_.get(buffer, start, length);
//    }
//
//    virtual void reset()
//    {
//        data_.reset();
//    }
//};





template <typename T>
class MemBuffer: public IDataSource<T> {
protected:
    SizeT   start_;
    SizeT   length_;
    T*      data_;
    bool    owner_;
public:

    MemBuffer(T* data, SizeT length, bool owner = false):
        start_(0),
        length_(length),
        data_(data),
        owner_(owner)
    {}

    MemBuffer(vector<T>& data):
        start_(0),
        length_(data.size()),
        data_(&data[0]),
        owner_(false)
    {}

    MemBuffer(SizeT length):
        start_(0),
        length_(length),
        data_(T2T<T*>(::malloc(length * sizeof(T)))),
        owner_(true)
    {}

    MemBuffer(MemBuffer<T>&& other):
        start_(other.start_),
        length_(other.length_),
        data_(other.data_),
        owner_(other.owner_)
    {
        other.data_ = NULL;
    }

    MemBuffer(const MemBuffer<T>& other):
        start_(other.start_),
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

    virtual SizeT skip(SizeT length)
    {
        if (start_ + length <= length_)
        {
            start_ += length;
            return length;
        }

        SizeT distance = length_ - start_;
        start_ = length_;
        return distance;
    }

    virtual SizeT getStart() const
    {
        return start_;
    }

    virtual SizeT getRemainder() const
    {
        return length_ - start_;
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

    SizeT size() const
    {
        return getSize();
    }

    virtual void setSize(SizeT size)
    {
        length_ = size;
    }

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        CopyBuffer(buffer + start, data_ + start_, length);

        return skip(length);
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
    	CopyBuffer(data_ + start_, buffer + start, length);

        return skip(length);
    }

    void dump(std::ostream& out) const {
        dumpArray(out, data_, length_);
    }

    virtual void reset()
    {
        start_ = 0;
    }
};

template <typename T>
class MemTBuffer: public IDataTarget<T> {
protected:
    SizeT   start_;
    SizeT   length_;
    T*      data_;
    bool    owner_;
public:

    MemTBuffer(T* data, SizeT length, bool owner = false):
        start_(0),
        length_(length),
        data_(data),
        owner_(owner)
    {}

    MemTBuffer(vector<T>& data):
        start_(0),
        length_(data.size()),
        data_(&data[0]),
        owner_(false)
    {}

    MemTBuffer(SizeT length):
        start_(0),
        length_(length),
        data_(T2T<T*>(::malloc(length * sizeof(T)))),
        owner_(true)
    {}

    MemTBuffer(MemTBuffer<T>&& other):
        start_(other.start_),
        length_(other.length_),
        data_(other.data_),
        owner_(other.owner_)
    {
        other.data_ = NULL;
    }

    MemTBuffer(const MemTBuffer<T>& other):
        start_(other.start_),
        length_(other.length_),
        owner_(true)
    {
        data_ = T2T<T*>(::malloc(length_*sizeof(T)));

        CopyBuffer(other.data(), data_, length_);
    }

    virtual ~MemTBuffer() throw ()
    {
        if (owner_) ::free(data_);
    }

    virtual SizeT skip(SizeT length)
    {
        if (start_ + length <= length_)
        {
            start_ += length;
            return length;
        }

        SizeT distance = length_ - start_;
        start_ = length_;
        return distance;
    }

    virtual SizeT getStart() const
    {
        return start_;
    }

    virtual SizeT getRemainder() const
    {
        return length_ - start_;
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

    SizeT size() const
    {
        return getSize();
    }

    virtual void setSize(SizeT size)
    {
        length_ = size;
    }

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        CopyBuffer(buffer + start, data_ + start_, length);

        return skip(length);
    }

//    virtual SizeT get(T* buffer, SizeT start, SizeT length)
//    {
//        CopyBuffer(data_ + start_, buffer + start, length);
//
//        return skip(length);
//    }

    void dump(std::ostream& out) const {
        dumpArray(out, data_, length_);
    }

    virtual void reset()
    {
        start_ = 0;
    }
};




template <typename T>
class MemBuffer<const T>: public IDataSource<T> {
protected:
    SizeT       start_;
    SizeT       length_;
    const T*    data_;
public:

    MemBuffer(const T* data, SizeT length):
        start_(0),
        length_(length),
        data_(data)
    {}

    MemBuffer(const MemBuffer<const T>& other):
        start_(other.start_),
        data_(other.data_),
        length_(other.length_)
    {}

    virtual ~MemBuffer() throw () {}

    virtual SizeT skip(SizeT length)
    {
        if (start_ + length <= length_)
        {
            start_ += length;
            return length;
        }

        SizeT distance = length_ - start_;
        start_ = length_;
        return distance;
    }

    virtual SizeT getStart() const
    {
        return start_;
    }

    virtual SizeT getRemainder() const
    {
        return length_ - start_;
    }

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

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        return 0;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        CopyBuffer(data_ + start_, buffer + start, length);

        return skip(length);
    }

    void dump(std::ostream& out) const
    {
        dumpArray(out, data_, length_);
    }

    virtual void reset()
    {
        start_ = 0;
    }
};



//template <typename T>
//class VariableRef: public IDataST<T> {
//protected:
//    SizeT   start_;
//    T&      value_;
//public:
//    VariableRef(T& value): start_(0), value_(value) {}
//
//    virtual ~VariableRef() throw () {}
//
//    virtual SizeT skip(SizeT length)
//    {
//        if (start_ + length <= 1)
//        {
//            start_ += 1;
//            return 1;
//        }
//
//        return 0;
//    }
//
//    virtual SizeT getStart() const
//    {
//        return start_;
//    }
//
//    virtual SizeT getRemainder() const
//    {
//        return 1 - start_;
//    }
//
//    virtual SizeT getSize() const
//    {
//        return 1;
//    }
//
//    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
//    {
//        value_ = *(buffer + start);
//        return skip(1);
//    }
//
//    virtual SizeT get(T* buffer, SizeT start, SizeT length)
//    {
//        *(buffer + start) = value_;
//        return skip(1);
//    }
//
//    operator T() const {
//        return value_;
//    }
//
//    operator const T&() const {
//        return value_;
//    }
//
//    operator T&() {
//        return value_;
//    }
//
//    virtual void reset()
//    {
//        start_ = 0;
//    }
//};
//
//template <typename T>
//class VariableRef<const T>: public IDataST<T> {
//protected:
//    SizeT       start_;
//    const T&    value_;
//public:
//    VariableRef(const T& value): start_(0), value_(value) {}
//
//    virtual ~VariableRef() throw () {}
//
//    virtual SizeT skip(SizeT length)
//    {
//        if (start_ + length <= 1)
//        {
//            start_ += 1;
//            return 1;
//        }
//
//        return 0;
//    }
//
//    virtual SizeT getStart() const
//    {
//        return start_;
//    }
//
//    virtual SizeT getRemainder() const
//    {
//        return 1 - start_;
//    }
//
//    virtual SizeT getSize() const
//    {
//        return 1;
//    }
//
//    virtual SizeT put(const T* buffer, SizeT start, SizeT length) {
//        return 0;
//    }
//
//    virtual SizeT get(T* buffer, SizeT start, SizeT length)
//    {
//        *(buffer + start) = value_;
//
//        return skip(1);
//    }
//
//    operator T() const {
//        return value_;
//    }
//
//    operator const T&() const {
//        return value_;
//    }
//
//    virtual void reset()
//    {
//        start_ = 0;
//    }
//};
//
//
//template <typename T>
//class VariableValue: public IDataST<T> {
//protected:
//    SizeT start_;
//    T value_;
//public:
//    VariableValue(): start_(0), value_() {}
//
//    VariableValue(const T& value): start_(0), value_(value) {}
//
//    virtual ~VariableValue() throw () {}
//
//    virtual SizeT skip(SizeT length)
//    {
//        if (start_ + length <= 1)
//        {
//            start_ += 1;
//            return 1;
//        }
//
//        return 0;
//    }
//
//    virtual SizeT getStart() const
//    {
//        return start_;
//    }
//
//    virtual SizeT getRemainder() const
//    {
//        return 1 - start_;
//    }
//
//    virtual SizeT getSize() const
//    {
//        return 1;
//    }
//
//    virtual void setSize(SizeT size) {}
//
//    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
//    {
//        value_ = *(buffer + start);
//        return skip(1);
//    }
//
//    virtual SizeT get(T* buffer, SizeT start, SizeT length)
//    {
//        *(buffer + start) = value_;
//        return skip(1);
//    }
//
//    operator T() const {
//        return value_;
//    }
//
//    operator const T&() const {
//        return value_;
//    }
//
//    operator T&() {
//        return value_;
//    }
//
//    virtual void reset()
//    {
//        start_ = 0;
//    }
//};


template <typename T>
class EmptyDataSource: public IDataSource<T> {
public:
	virtual SizeT get(T* buffer, SizeT start, SizeT length) {return 0;}

    virtual SizeT skip(SizeT length) {return 0;}
    virtual SizeT getStart() const {return 0;}
    virtual SizeT getRemainder() const {return 0;}
    virtual SizeT getSize() const {return 0;}
    virtual void  reset() {}
};

template <typename T>
class EmptyDataTarget: public IDataTarget<T> {
public:
	virtual SizeT put(const T* buffer, SizeT start, SizeT length) {return 0;}

    virtual SizeT skip(SizeT length) {return 0;}
    virtual SizeT getStart() const {return 0;}
    virtual SizeT getRemainder() const {return 0;}
    virtual SizeT getSize() const {return 0;}
    virtual void  reset() {}
};


template <typename T>
class IStreamDataWrapper: public IDataSource<T> {
    istream& is_;
    SizeT start_;
    SizeT length_;
public:
    IStreamDataWrapper(istream& is, SizeT length): is_(is), start_(0), length_(length) {}

    virtual SizeT skip(SizeT length)
    {
        if (start_ + length <= length_)
        {
            start_ += length;
            return length;
        }

        SizeT distance = length_ - start_;

        start_ = length_;

        return distance;
    }

    virtual SizeT getStart() const
    {
        return start_;
    }

    virtual SizeT getRemainder() const
    {
        return length_ - start_;
    }

    virtual SizeT getSize() const {return length_;}

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        return 0;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        for (SizeT c = start; c < start + length; c++)
        {
            is_>>buffer[c];
        }

        // FIXME: EOF handling?
        return skip(length);
    }

    virtual void reset()
    {
        start_ = 0;
    }
};

template <typename T>
class OStreamDataWrapper: public IDataTarget<T> {
    ostream& os_;
    SizeT start_;
    SizeT length_;
public:
    OStreamDataWrapper(ostream& os, SizeT length): os_(os), start_(0), length_(length) {}

    virtual SizeT skip(SizeT length)
    {
        if (start_ + length <= length_)
        {
            start_ += length;
            return length;
        }

        SizeT distance = length_ - start_;
        start_ = length_;
        return distance;
    }

    virtual SizeT getStart() const
    {
        return start_;
    }

    virtual SizeT getRemainder() const
    {
        return length_ - start_;
    }

    virtual SizeT getSize() const {return length_;}

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        for (SizeT c = start; c < start + length; c++)
        {
            os_<<buffer[c];
        }

        // FIXME EOF handling?
        return skip(length);
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        return 0;
    }

    virtual void reset()
    {
        start_ = 0;
    }
};


}
}



#endif
