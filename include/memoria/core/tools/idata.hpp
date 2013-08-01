
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_IDATA_HPP
#define _MEMORIA_CORE_TOOLS_IDATA_HPP


#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <vector>
#include <iostream>
#include <vector>
#include <tuple>

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

				Int idx = (c + d) * bits_per_symbol;

				if (sizeof(T) > 1) {
					out_<<GetBits(symbols, idx, bits_per_symbol);
				}
				else {
					out_<<(Int)GetBits(symbols, idx, bits_per_symbol);
				}
			}

			out_<<dec<<endl;
		}
	} while (c < size_);
}

enum class IDataAPI {
	Batch = 1, Single = 2, Both = 3
};

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

    virtual IDataAPI api() const									= 0;

    virtual SizeT skip(SizeT length)                                = 0;
    virtual SizeT getStart() const                                  = 0;
    virtual SizeT getRemainder() const                              = 0;
    virtual SizeT getSize() const                                   = 0;
    virtual SizeT getAdvance() const                                = 0;
    virtual void  reset()                                           = 0;
};



template <typename T>
struct IDataSource: IDataBase {

    virtual ~IDataSource() throw () {}

    virtual SizeT get(T* buffer, SizeT start, SizeT length)               		= 0;
    virtual T get()																= 0;
};


template <typename T>
struct IDataTarget: IDataBase {

    virtual ~IDataTarget() throw () {}

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)               = 0;
    virtual void put(const T& value)											= 0;
};


template <typename T>
class DataSourceProxy: public IDataSource<T> {
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

	virtual IDataAPI api() const {
		return data_.api();
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

    virtual SizeT getAdvance() const
    {
    	SizeT remainder = getRemainder();
    	SizeT advance 	= data_.getAdvance();

    	return advance <= remainder ? advance : remainder;
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

    virtual T get()	{
    	return data_.get();
    }
};







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

    virtual IDataAPI api() const
    {
    	return IDataAPI::Both;
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

    virtual SizeT getAdvance() const
    {
    	return getRemainder();
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
    	MEMORIA_ASSERT_TRUE(start_ + length <= length_);

    	CopyBuffer(buffer + start, data_ + start_, length);

        return skip(length);
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
    	MEMORIA_ASSERT_TRUE(start_ + length <= length_);

    	CopyBuffer(data_ + start_, buffer + start, length);

        return skip(length);
    }

    virtual T get()
    {
    	T value = *(data_ + start_);

    	skip(1);

    	return value;
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

    virtual IDataAPI api() const
    {
    	return IDataAPI::Both;
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

    virtual SizeT getAdvance() const
    {
    	return getRemainder();
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

    virtual void put(const T& value)
    {
    	*(data_ + start_) = value;
    	skip(1);
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

    virtual IDataAPI api() const
    {
    	return IDataAPI::Both;
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

    virtual SizeT getAdvance() const
    {
    	return getRemainder();
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


    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        CopyBuffer(data_ + start_, buffer + start, length);

        return skip(length);
    }

    virtual T get()
    {
    	T value = *(data_ + start_);

    	skip(1);

    	return value;
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


template <typename Value>
class ValueSource: public IDataSource<Value> {
	const Value& value_;

	SizeT start_ 	= 0;
	SizeT length_ 	= 1;

public:
	ValueSource(const Value& value): value_(value) {}

	virtual IDataAPI api() const
	{
		return IDataAPI::Both;
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

	virtual SizeT getAdvance() const
	{
		return getRemainder();
	}

	virtual SizeT getSize() const
	{
		return length_;
	}

	SizeT size() const
	{
		return getSize();
	}

	virtual void setSize(SizeT size)
	{
		length_ = size;
	}

	virtual SizeT get(Value* buffer, SizeT start, SizeT length)
	{
		MEMORIA_ASSERT_TRUE(start_ + length <= length_);

		CopyBuffer(&value_ + start_, buffer + start, length);

		return skip(length);
	}

	virtual Value get()
	{
		skip(1);

		return value_;
	}

	virtual void reset()
	{
		start_ = 0;
	}
};


template <typename Value>
class ValueTarget: public IDataSource<Value> {
	Value& value_;

	SizeT start_ 	= 0;
	SizeT length_ 	= 1;

public:
	ValueTarget(Value& value): value_(value) {}

	virtual IDataAPI api() const
	{
		return IDataAPI::Both;
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

	virtual SizeT getAdvance() const
	{
		return getRemainder();
	}

	virtual SizeT getSize() const
	{
		return length_;
	}

	SizeT size() const
	{
		return getSize();
	}

	virtual void setSize(SizeT size)
	{
		length_ = size;
	}

    virtual SizeT put(const Value* buffer, SizeT start, SizeT length)
    {
        CopyBuffer(buffer + start, &value_ + start_, length);

        return skip(length);
    }

    virtual void put(const Value& value)
    {
    	value_ = value;
    	skip(1);
    }

	virtual void reset()
	{
		start_ = 0;
	}
};





template <typename T>
class EmptyDataSource: public IDataSource<T> {
public:
	virtual SizeT get(T* buffer, SizeT start, SizeT length) {return 0;}
    virtual T get() {throw Exception(MA_SRC, "Unsupported operation");}


	virtual IDataAPI api() const
	{
		return IDataAPI::Both;
	}

    virtual SizeT skip(SizeT length) {return 0;}
    virtual SizeT getStart() const {return 0;}
    virtual SizeT getRemainder() const {return 0;}
    virtual SizeT getAdvance() const {return 0;}
    virtual SizeT getSize() const {return 0;}
    virtual void  reset() {}
};


template <typename T>
class EmptyDataTarget: public IDataTarget<T> {
public:
	virtual SizeT put(const T* buffer, SizeT start, SizeT length) {return 0;}
	virtual void put(const T&) {throw Exception(MA_SRC, "Unsupported operation");}

	virtual IDataAPI api() const
	{
		return IDataAPI::Both;
	}

    virtual SizeT skip(SizeT length) {return 0;}
    virtual SizeT getStart() const {return 0;}
    virtual SizeT getRemainder() const {return 0;}
    virtual SizeT getAdvance() const {return 0;}
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

    virtual IDataAPI api() const
    {
    	return IDataAPI::Both;
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

    virtual SizeT getAdvance() const
    {
    	return getRemainder();
    }

    virtual SizeT getSize() const {return length_;}


    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        for (SizeT c = start; c < start + length; c++)
        {
            is_>>buffer[c];
        }

        // FIXME: EOF handling?
        return skip(length);
    }

    virtual T get()
    {
    	T value;
    	is_>>value;

    	// FIXME: EOF handling?
    	skip(1);

    	return value;
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

    virtual IDataAPI api() const
    {
    	return IDataAPI::Both;
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

    virtual SizeT getAdvance() const
    {
    	return getRemainder();
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

    virtual void put(const T& value)
    {
    	os_<<value;

    	// FIXME EOF handling?
    	skip(1);
    }

    virtual void reset()
    {
        start_ = 0;
    }
};





}
}



#endif
