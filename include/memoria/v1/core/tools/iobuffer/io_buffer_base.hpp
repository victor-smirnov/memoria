
// Copyright 2016 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/algo/select.hpp>
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/bytes/bytes_codec.hpp>
#include <memoria/v1/core/tools/bignum/primitive_codec.hpp>
#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/packed/sseq/rleseq/rleseq_tools.hpp>

#include <limits>
#include <cstring>

#include <malloc.h>

namespace memoria {
namespace v1 {

template <ByteOrder Endian, MemoryAccess AccessType = MemoryAccess::MMA_UNALIGNED> class IOBuffer;

class IOBufferBase {
protected:
	static constexpr size_t MARK_MAX = std::numeric_limits<size_t>::max();


    UByte* array_ 	= nullptr;
    size_t length_	= 0;
    size_t limit_ 	= 0;
    size_t pos_ 	= 0;
    size_t mark_ 	= MARK_MAX;

    bool owner_;

    ValueCodec<UBigInt> uvlen_codec_;
    ValueCodec<BigInt>  vlen_codec_;
    ValueCodec<String>  string_codec_;
    ValueCodec<Bytes>   bytes_codec_;

public:
    
    template <typename T>
    using ValueSize = IfThenElse<
        std::is_same<T, bool>::value,
        std::integral_constant<size_t, 1>,
        std::integral_constant<size_t, sizeof(T)>
    >;
    

    IOBufferBase():
        array_(nullptr), owner_(false)
	{}

    IOBufferBase(size_t length):
        array_(allocate(length)), length_(length), limit_(length), owner_(true)
    {}

    IOBufferBase(UByte* data, size_t length):
        array_(data), length_(length), limit_(length), owner_(false)
    {}

    IOBufferBase(UByte* data, size_t pos, size_t length):
    	array_(data), length_(length), limit_(length), pos_(pos), owner_(false)
    {}

    IOBufferBase(const IOBufferBase&) = delete;

    IOBufferBase(IOBufferBase&& other):
        array_(other.array_), length_(other.length_), limit_(other.limit_), pos_(other.pos_), mark_(other.mark_), owner_(true)
    {
        other.array_ = nullptr;
        other.owner_ = false;
    }

    ~IOBufferBase() {
        release();
    }
    

    UByte* array() {
        return array_;
    }

    const UByte* array() const {
        return array_;
    }

    UByte* ptr() {
        return array_ + pos_;
    }

    const UByte* ptr() const {
        return array_ + pos_;
    }

    void rewind()
    {
        pos_ = 0;

        mark_  = MARK_MAX;
        limit_ = length_;
    }

    void clear() {
        rewind();
    }

    size_t pos() const {
        return pos_;
    }

    void pos(size_t v)
    {
        if (v <= limit_)
        {
        	pos_ = v;
        }
        else {
        	throw Exception(MA_SRC, SBuf() << "Supplied value of position exceeds limit: " << v << " " << limit_);
        }
    }

    void skip(size_t len)
    {
    	if (pos_ + len <= limit_)
    	{
    		pos_ += len;
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Supplied value of len exceeds limit: " << len << " " << pos_ << " " << limit_);
    	}
    }

    size_t capacity() const {
        return limit_ - pos_;
    }

    bool has_capacity(size_t size) const {
        return capacity() >= size;
    }

    size_t limit() const {
    	return limit_;
    }

    void limit(size_t value)
    {
    	if (value <= length_)
    	{
    		limit_ = value;
    	}
        else {
            throw Exception(MA_SRC, SBuf() << "Supplied value of limit exceeds capacity: " << value << " " << length_);
        }
    }

    size_t size() const {
    	return length_;
    }

    void done() {
    	pos_ = limit_;
    }

    void flip()
    {
    	limit_ = pos_;
    	pos_   = 0;

    	mark_  = MARK_MAX;
    }

    size_t  mark() {
    	return mark_ = pos_;
    }

    void reset()
    {
    	if (mark_ != MARK_MAX)
    	{
    		pos_ = mark_;
    	}
    	else {
    		throw Exception(MA_SRC, "IObuffer is not marked");
    	}
    }


    void move(size_t from, size_t to, size_t length)
    {
    	MEMORIA_V1_ASSERT(from, <=, limit_);
    	MEMORIA_V1_ASSERT(to, <=, limit_);

    	MEMORIA_V1_ASSERT(from + length, <=, limit_);
    	MEMORIA_V1_ASSERT(to + length, <=, limit_);

    	CopyBuffer(array_ + from, array_ + to, length);
    }

    void moveRemainingToStart()
    {
    	CopyBuffer(array_ + pos_, array_, limit_ - pos_);

    	pos_ 	= limit_ - pos_;
    	limit_	= length_;
    	mark_ 	= MARK_MAX;
    }
    
    void moveRemainingToStart0()
    {
    	std::memmove(array_, array_ + pos_, limit_ - pos_);

        limit_	-= pos_;
    	pos_ 	= 0;
    	
    	mark_ 	= MARK_MAX;
    }

    bool put(Byte v)
    {
        if (has_capacity(1))
        {
            array_[pos_++] = v;
            return true;
        }
        else {
            return false;
        }
    }

    Byte getByte()
    {
        assertRange(1, "getByte()");
        return (Byte)array_[pos_++];
    }

    bool put(UByte v)
    {
        if (has_capacity(1))
        {
            array_[pos_++] = v;
            return true;
        }
        else {
            return false;
        }
    }

    UByte getUByte()
    {
        assertRange(1, "getByte()");
        return (UByte)array_[pos_++];
    }

    bool put(Char v)
    {
        if (has_capacity(1))
        {
            array_[pos_++] = v;
            return true;
        }
        else {
            return false;
        }
    }

    UByte getChar()
    {
        assertRange(1, "getChar()");
        return (Char)array_[pos_++];
    }

    bool put(bool v)
    {
        if (has_capacity(1))
        {
            array_[pos_++] = v;
            return true;
        }
        else {
            return false;
        }
    }

    bool getBool()
    {
        assertRange(1, "getBool()");
        return (bool)array_[pos_++];
    }

    bool put(const Bytes& bytes)
    {
        if (has_capacity(bytes_codec_.length(bytes)))
        {
            pos_ += bytes_codec_.encode(array_, bytes, pos_);
            return true;
        }
        else {
            return false;
        }
    }

    Bytes getBytes()
    {
        int64_t len = 0;
        pos_ += vlen_codec_.encode(array_, len, pos_);

        Bytes bytes(array_ + pos_, len, false);

        pos_ += bytes.length();

        return bytes;
    }

    bool put(const String& str)
    {
        if (has_capacity(string_codec_.length(str)))
        {
            pos_ += string_codec_.encode(array_, str, pos_);
            return true;
        }
        else {
            return false;
        }
    }

    String getString()
    {
        String str;
        pos_ += string_codec_.decode(array_, str, pos_);
        return str;
    }

    size_t length(BigInt val) const
    {
        return vlen_codec_.length(val);
    }

    size_t ulength(UBigInt val) const
    {
        return uvlen_codec_.length(val);
    }

    bool putVLen(BigInt val)
    {
        size_t len = vlen_codec_.length(val);
        if (has_capacity(len))
        {
            pos_ += vlen_codec_.encode(array_, val, pos_);
            return true;
        }
        else {
            return false;
        }
    }

    bool putUVLen(UBigInt val)
    {
        size_t len = uvlen_codec_.length(val);
        if (has_capacity(len))
        {
            pos_ += uvlen_codec_.encode(array_, val, pos_);
            return true;
        }
        else {
            return false;
        }
    }


    BigInt getVLen()
    {
        BigInt len = 0;
        pos_ += vlen_codec_.decode(array_, len, pos_);
        return len;
    }

    BigInt getUVLen()
    {
        UBigInt len = 0;
        pos_ += uvlen_codec_.decode(array_, len, pos_);
        return len;
    }

    bool put(const UByte* data, size_t length)
    {
        if (MMA1_LIKELY(has_capacity(length)))
        {
            //CopyBuffer(data, array_ + pos_, length);
            std::memcpy(array_ + pos_, data, length);
            pos_ += length;
            return true;
        }
        else {
            return false;
        }
    }
    
    bool put_(const UByte* data, size_t length)
    {
        std::memcpy(array_ + pos_, data, length);
        pos_ += length;
        return true;
    }

    void get(UByte* data, size_t length)
    {
        assertRange(length, "get(UByte*, size_t)");
        //CopyBuffer(array_ + pos_, data, length);
        std::memcpy(data, array_ + pos_, length);
        pos_ += length;
    }
    
    void get_(UByte* data, size_t length)
    {
        std::memcpy(data, array_ + pos_, length);
        pos_ += length;
    }


    bool put(const ValuePtrT1<UByte>& value)
    {
        if (has_capacity(value.length()))
        {
            CopyBuffer(value.addr(), array_ + pos_, value.length());
            pos_ += value.length();
            return true;
        }
        else {
            return false;
        }
    }


    template <Int Symbols>
    static constexpr BigInt getMaxSymbolsRunLength()
    {
        return MaxRLERunLength;
    }

    template <Int Symbols>
    bool putSymbolsRun(Int symbol, UBigInt length)
    {
        if (length <= getMaxSymbolsRunLength<Symbols>())
        {
            UBigInt value = memoria::v1::rleseq::EncodeRun<Symbols, MaxRLERunLength>(symbol, length);
            return putUVLen(value);
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Max symbols run length of " << length << " exceeds " << getMaxSymbolsRunLength<Symbols>());
        }
    }


    template <Int Symbols>
    void updateSymbolsRun(size_t pos, Int symbol, UBigInt length)
    {
        if (length <= getMaxSymbolsRunLength<Symbols>())
        {
            auto current_length     = uvlen_codec_.length(array_, pos);
            auto new_value          = memoria::v1::rleseq::EncodeRun<Symbols, MaxRLERunLength>(symbol, length);
            auto new_length         = uvlen_codec_.length(new_value);

            if (new_length > current_length)
            {
                auto delta = new_length - current_length;
                insert_space(pos, delta);
            }
            else {
                auto delta = current_length - new_length;
                remove_space(pos, delta);
            }

            uvlen_codec_.encode(array_, new_value, pos);
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Max symbols run length of " << length << " exceeds " << getMaxSymbolsRunLength<Symbols>());
        }
    }


    template <Int Symbols>
    memoria::v1::rleseq::RLESymbolsRun getSymbolsRun()
    {
        UBigInt value = getUVLen();
        return memoria::v1::rleseq::DecodeRun<Symbols>(value);
    }

    void enlarge(size_t minimal_capacity = 0)
    {
        if (owner_)
        {
            size_t new_length   = length_ * 2;
            size_t new_capacity = new_length - pos_;

            if (new_capacity < minimal_capacity)
            {
                new_length += minimal_capacity - new_capacity;
            }

            auto new_array = allocate(new_length);

            CopyBuffer(array_, new_array, pos_);

            ::free(array_);

            array_  = new_array;
            length_ = new_length;
        }
        else {
            throw Exception(MA_SRC, "IOBuffer can't enlarge alien data buffer");
        }
    }

    void assertRange(size_t window, const char* op_type)
    {
        if (pos_ + window <= limit_)
        {
            return;
        }
        else {
            throw Exception(MA_SRC, SBuf() << "IOBuffer::" << op_type << " is out of bounds: " << pos_ << " " << window << " " << limit_);
        }
    }

protected:

    void insert_space(size_t pos, size_t length)
    {
        MEMORIA_V1_ASSERT(pos, <=, limit_);


        auto available = limit_ - pos_;

        if (length < available)
        {
            CopyBuffer(array_ + pos, array_ + pos + length, pos_ - pos);

            limit_ += length;

            if (pos < pos_)
            {
            	pos_ += length;
            }

            if (mark_ < MARK_MAX && mark_ > pos)
            {
            	mark_ += length;
            }
        }
        else {
            throw Exception(MA_SRC, SBuf() << "IOBuffer has no enough free space: " << length << " " << available);
        }
    }

    void remove_space(size_t pos, size_t length)
    {
        MEMORIA_V1_ASSERT(pos, <=, limit_);
        MEMORIA_V1_ASSERT(pos + length, <=, limit_);

        CopyBuffer(array_ + pos + length, array_ + pos, pos_ - (pos + length));

        if (pos + length <= pos_)
        {
        	pos_ -= length;
        }
        else if (pos_ >= pos) {
        	pos_ = pos;
        }

        if (mark_ < MARK_MAX && mark_ > pos)
        {
        	mark_ -= length;
        }

        limit_ -= length;

        if (pos_ > limit_)
        {
        	pos_ = limit_;
        }
    }

    void release()
    {
        if (owner_ && array_) {
            ::free(array_);
        }
    }

    static UByte* allocate(size_t length)
    {
        UByte* data = T2T<UByte*>(::malloc(length));

        if (data)
        {
            return data;
        }
        else {
            throw Exception(MA_SRC, "Can't allocate IOBuffer");
        }
    }
};





}
}
