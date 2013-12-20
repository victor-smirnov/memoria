
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

enum class IDataAPI: Int {
    None = 0, Batch = 1, Single = 2, Both = 3
};

inline constexpr IDataAPI operator&(IDataAPI m1, IDataAPI m2)
{
    return static_cast<IDataAPI>(static_cast<Int>(m1) & static_cast<Int>(m2));
}

inline constexpr IDataAPI operator|(IDataAPI m1, IDataAPI m2)
{
    return static_cast<IDataAPI>(static_cast<Int>(m1) | static_cast<Int>(m2));
}

inline constexpr bool to_bool(IDataAPI mode) {
    return static_cast<Int>(mode) > 0;
}





struct IData {};

struct INodeLayoutManager {
    virtual Int getNodeCapacity(const Int* sizes, Int stream)       = 0;
};

struct ISource {
    virtual Int streams()                                           = 0;
    virtual IData* stream(Int stream)                               = 0;

    virtual void newNode(INodeLayoutManager* layout_manager, BigInt* sizes) = 0;
    virtual BigInt getTotalNodes(INodeLayoutManager* layout_manager)= 0;
};

struct ITarget {
    virtual Int streams()                                           = 0;
    virtual IData* stream(Int stream)                               = 0;
};





struct IDataBase: IData {

    virtual ~IDataBase() throw () {}

    virtual IDataAPI api() const                                    = 0;

    virtual SizeT skip(SizeT length)                                = 0;
    virtual SizeT getStart() const                                  = 0;
    virtual SizeT getRemainder() const                              = 0;
    virtual SizeT getSize() const                                   = 0;
    virtual SizeT getAdvance() const                                = 0;
    virtual void  reset(BigInt pos = 0)                             = 0;
};



template <typename T>
struct IDataSource: IDataBase {
    typedef T Type;

    virtual ~IDataSource() throw () {}

    virtual SizeT get(T* buffer, SizeT start, SizeT length)                     = 0;
    virtual T get()                                                             = 0;
};


template <typename T>
struct IDataTarget: IDataBase {

    virtual ~IDataTarget() throw () {}

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)               = 0;
    virtual void put(const T& value)                                            = 0;
    virtual T peek()                                            				= 0;
};


template <typename T>
class DataSourceProxy: public IDataSource<T> {
    typedef IDataSource<T>                  Base;
    typedef Base                            Delegate;

    Delegate&   data_;
    BigInt      size_;
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
        SizeT advance   = data_.getAdvance();

        return advance <= remainder ? advance : remainder;
    }

    virtual SizeT getSize() const
    {
        return size_;
    }

    virtual void reset(BigInt pos = 0)
    {
        data_.reset(pos);
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        return data_.get(buffer, start, length);
    }

    virtual T get() {
        return data_.get();
    }
};





template <typename T>
class AbstractData: public IDataSource<T>, public IDataTarget<T> {
protected:
    SizeT   start_;
    SizeT   length_;

public:

    AbstractData(SizeT start, SizeT length):
        start_(start),
        length_(length)
    {}

    virtual ~AbstractData() throw ()
    {}



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

    virtual IDataAPI api() const                                                = 0;
    virtual SizeT put(const T* buffer, SizeT start, SizeT length)               = 0;
    virtual SizeT get(T* buffer, SizeT start, SizeT length)                     = 0;
    virtual T peek()                                            				= 0;


    virtual T get()                                                             = 0;
    virtual void put(const T& value)                                            = 0;

    virtual void reset(BigInt pos = 0)
    {
        start_ = pos;
    }
};


template <typename T>
class AbstractDataSource: public IDataSource<T> {
protected:
    SizeT   start_;
    SizeT   length_;

public:

    AbstractDataSource(SizeT start, SizeT length):
        start_(start),
        length_(length)
    {}

    virtual ~AbstractDataSource() throw ()
    {}



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

    virtual IDataAPI api() const                                                = 0;

    virtual SizeT get(T* buffer, SizeT start, SizeT length)                     = 0;
    virtual T get()                                                             = 0;


    virtual void reset(BigInt pos = 0)
    {
        start_ = pos;
    }
};



template <typename T>
class AbstractDataTarget: public IDataTarget<T> {
protected:
    SizeT   start_;
    SizeT   length_;

public:

    AbstractDataTarget(SizeT start, SizeT length):
        start_(start),
        length_(length)
    {}

    virtual ~AbstractDataTarget() throw () {}



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

    virtual IDataAPI api() const                                                = 0;
    virtual SizeT put(const T* buffer, SizeT start, SizeT length)               = 0;
    virtual void put(const T& value)                                            = 0;
    virtual T peek()                                            				= 0;

    virtual void reset(BigInt pos = 0)
    {
        start_ = pos;
    }
};









template <typename T>
class MemBuffer: public AbstractData<T> {
protected:

    typedef AbstractData<T>                                                     Base;

    T*      data_;
    bool    owner_;

public:

    MemBuffer(T* data, SizeT length, bool owner = false):
        Base(0, length),
        data_(data),
        owner_(owner)
    {}

    MemBuffer(vector<T>& data):
        Base(0, data.size()),
        data_(&data[0]),
        owner_(false)
    {}

    MemBuffer(SizeT length):
        Base(0, length),
        data_(T2T<T*>(::malloc(length * sizeof(T)))),
        owner_(true)
    {}

    MemBuffer(MemBuffer<T>&& other):
        Base(other.start_, other.length_),
        data_(other.data_),
        owner_(other.owner_)
    {
        other.data_ = NULL;
    }

    MemBuffer(const MemBuffer<T>& other):
        Base(other.start_, other.length_),
        owner_(true)
    {
        data_ = T2T<T*>(::malloc(Base::length_*sizeof(T)));

        CopyBuffer(other.data(), data_, Base::length_);
    }

    virtual ~MemBuffer() throw ()
    {
        if (owner_) {
            ::free(data_);
        }
    }

    virtual IDataAPI api() const
    {
        return IDataAPI::Both;
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
        return Base::getSize();
    }

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        MEMORIA_ASSERT_TRUE(Base::start_ + length <= Base::length_);

        CopyBuffer(buffer + start, data_ + Base::start_, length);

        return Base::skip(length);
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        MEMORIA_ASSERT_TRUE(Base::start_ + length <= Base::length_);

        CopyBuffer(data_ + Base::start_, buffer + start, length);

        return Base::skip(length);
    }

    virtual T get()
    {
        T value = *(data_ + Base::start_);

        Base::skip(1);

        return value;
    }

    virtual void put(const T& value)
    {
        *(data_ + Base::start_) = value;
        Base::skip(1);
    }

    virtual T peek()
    {
    	return *(data_ + Base::start_);
    }

    void dump(std::ostream& out) const
    {
        dumpArray(out, data_, Base::length_);
    }
};



template <typename T>
class MemBuffer<const T>: public AbstractDataSource<T> {
protected:
    typedef AbstractDataSource<T>                                               Base;

    const T*    data_;
public:

    MemBuffer(const T* data, SizeT length):
        Base(0, length),
        data_(data)
    {}

    MemBuffer(const MemBuffer<const T>& other):
        Base(other.start_, other.length_),
        data_(other.data_)
    {}

    MemBuffer(const vector<T>& data):
        Base(0, data.size()),
        data_(&data[0])
    {}

    virtual ~MemBuffer() throw () {}

    virtual IDataAPI api() const
    {
        return IDataAPI::Both;
    }

    const T* data() const
    {
        return data_;
    }

    SizeT size() const {
        return this->getSize();
    }


    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        CopyBuffer(data_ + this->start_, buffer + start, length);

        return this->skip(length);
    }

    virtual T get()
    {
        T value = *(data_ + this->start_);

        this->skip(1);

        return value;
    }

    void dump(std::ostream& out) const
    {
        dumpArray(out, data_, this->length_);
    }
};












template <typename Value>
class ValueSource: public AbstractDataSource<Value> {
protected:
    typedef AbstractDataSource<Value>                                           Base;

    const Value& value_;

public:
    ValueSource(const Value& value):
        Base(0, 1),
        value_(value) {}

    virtual IDataAPI api() const
    {
        return IDataAPI::Single;
    }

    SizeT size() const
    {
        return this->getSize();
    }

    virtual SizeT get(Value* buffer, SizeT start, SizeT length)
    {
        return 0;
    }

    virtual Value get()
    {
        this->skip(1);

        return value_;
    }
};


template <typename Value>
class ValueTarget: public AbstractDataTarget<Value> {
protected:

    typedef AbstractDataTarget<Value>                                           Base;

    Value& value_;

public:
    ValueTarget(Value& value): Base(0, 1), value_(value) {}

    virtual IDataAPI api() const
    {
        return IDataAPI::Single;
    }


    SizeT size() const
    {
        return this->getSize();
    }

    virtual SizeT put(const Value* buffer, SizeT start, SizeT length)
    {
       return 0;
    }

    virtual void put(const Value& value)
    {
        value_ = value;
        this->skip(1);
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
    virtual void  reset(SizeT pos = 0) {}
};


template <typename T>
class EmptyDataTarget: public IDataTarget<T> {
public:
    virtual SizeT put(const T* buffer, SizeT start, SizeT length) {return 0;}
    virtual void put(const T&) {throw Exception(MA_SRC, "Unsupported operation");}
    virtual T peek() {throw Exception(MA_SRC, "Unsupported operation");}

    virtual IDataAPI api() const
    {
        return IDataAPI::Both;
    }

    virtual SizeT skip(SizeT length) {return 0;}
    virtual SizeT getStart() const {return 0;}
    virtual SizeT getRemainder() const {return 0;}
    virtual SizeT getAdvance() const {return 0;}
    virtual SizeT getSize() const {return 0;}
    virtual void  reset(SizeT pos = 0) {}
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

    virtual void reset(SizeT pos = 0)
    {
        start_ = pos;
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

    virtual T peek() {throw Exception(MA_SRC, "Unsupported operation");}

    virtual void reset(SizeT pos = 0)
    {
        start_ = pos;
    }
};





}
}



#endif
