
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_ISYMBOLS_HPP
#define _MEMORIA_CORE_TOOLS_ISYMBOLS_HPP


#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/tools/accessors.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <memoria/core/tools/idata.hpp>

#include <vector>
#include <iostream>
#include <vector>
#include <tuple>

#include <malloc.h>

namespace memoria    {
namespace vapi       {

template <typename T1, typename T2>
constexpr static T2 DivUp0(T1 v, T2 d) {
    return v / d + (v % d > 0);
}

template <typename T, Int BitsPerSymbol>
SizeT RoundSymbolsToStorageType(SizeT length)
{
	SizeT bitsize 				= length * BitsPerSymbol;
	const SizeT item_bitsize 	= sizeof(T) * 8;

	SizeT result = DivUp0(bitsize, item_bitsize);

	return result * sizeof(T);
};

template <Int BitsPerSymbol, typename T = UBigInt>
class SymbolsBuffer: public IDataSource<T>, public IDataTarget<T> {
	typedef SymbolsBuffer<BitsPerSymbol, T>										MyType;
protected:
    SizeT   start_;
    SizeT   length_;
    T*      data_;
    bool    owner_;
public:

    typedef T value_type;

    static SizeT storage_size(SizeT length)
    {
    	return RoundSymbolsToStorageType<T, BitsPerSymbol>(length);
    }

    SymbolsBuffer(T* data, SizeT length, bool owner = false):
        start_(0),
        length_(length),
        data_(data),
        owner_(owner)
    {}

    SymbolsBuffer(SizeT length):
        start_(0),
        length_(length),
        data_(T2T<T*>(length > 0 ?::malloc(storage_size(length)) : nullptr)),
        owner_(true)
    {}

    SymbolsBuffer(MyType&& other):
        start_(other.start_),
        length_(other.length_),
        data_(other.data_),
        owner_(other.owner_)
    {
        other.data_ = NULL;
    }

    SymbolsBuffer(const MyType& other):
        start_(other.start_),
        length_(other.length_),
        owner_(true)
    {
        data_ = T2T<T*>(length_ > 0 ?::malloc(storage_size(length_)) : nullptr);

        CopyBuffer(other.data(), data_, length_);
    }

    virtual ~SymbolsBuffer() throw ()
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

        MoveBits(buffer, data_, start * BitsPerSymbol, start_ * BitsPerSymbol, length * BitsPerSymbol);

        return skip(length);
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        MEMORIA_ASSERT_TRUE(start_ + length <= length_);

        MoveBits(data_, buffer, start_ * BitsPerSymbol, start * BitsPerSymbol, length * BitsPerSymbol);

        return skip(length);
    }

    virtual T get()
    {
        T value = GetBits(data_, start_ * BitsPerSymbol, BitsPerSymbol);

        skip(1);

        return value;
    }



    virtual void put(const T& value)
    {
    	SetBits(data_, start_ * BitsPerSymbol, value, BitsPerSymbol);
    	skip(1);
    }


    void dump(std::ostream& out) const
    {
        dumpSymbols(out, data_, length_, BitsPerSymbol);
    }

    virtual void reset()
    {
        start_ = 0;
    }


    void clear()
    {
    	SizeT len = storage_size(length_) / sizeof(T);

    	for (SizeT c = 0; c < len; c++)
    	{
    		data_[c] = 0;
    	}
    }

    BitmapAccessor<T*, T, BitsPerSymbol> operator[](SizeT idx)
    {
    	return BitmapAccessor<T*, T, BitsPerSymbol>(data_, idx);
    }

    BitmapAccessor<const T*, T, BitsPerSymbol> operator[](SizeT idx) const
    {
    	return BitmapAccessor<const T*, T, BitsPerSymbol>(data_, idx);
    }
};







template <Int BitsPerSymbol, typename T>
class SymbolsBuffer<BitsPerSymbol, const T>: public IDataSource<T> {
	typedef SymbolsBuffer<BitsPerSymbol, T>										MyType;

protected:
    SizeT       start_;
    SizeT       length_;
    const T*    data_;
public:

    static SizeT storage_size(SizeT length)
    {
    	return RoundSymbolsToStorageType<T, BitsPerSymbol>(length);
    }

    SymbolsBuffer(const T* data, SizeT length):
        start_(0),
        length_(length),
        data_(data)
    {}

    SymbolsBuffer(const MyType& other):
        start_(other.start_),
        data_(other.data_),
        length_(other.length_)
    {}


    virtual ~SymbolsBuffer() throw () {}

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
        MoveBits(data_, buffer, start_ * BitsPerSymbol, start * BitsPerSymbol, length * BitsPerSymbol);

        return skip(length);
    }

    virtual T get()
    {
    	T value = GetBits(data_, start_ * BitsPerSymbol, BitsPerSymbol);

        skip(1);

        return value;
    }

    void dump(std::ostream& out) const
    {
        dumpSymbols(out, data_, length_, BitsPerSymbol);
    }

    virtual void reset()
    {
        start_ = 0;
    }
};


}
}



#endif
