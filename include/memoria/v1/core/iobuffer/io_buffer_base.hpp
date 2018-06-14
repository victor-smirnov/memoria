
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

#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/core/types/algo/select.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/bytes/bytes_codec.hpp>
#include <memoria/v1/core/bignum/primitive_codec.hpp>
#include <memoria/v1/core/strings/string_codec.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/packed/sseq/rleseq/rleseq_tools.hpp>

#include <memoria/v1/core/integer/integer.hpp>

#include <limits>
#include <cstring>

namespace memoria {
namespace v1 {

template <ByteOrder Endian, MemoryAccess AccessType = MemoryAccess::MMA_UNALIGNED> class IOBuffer;

class IOBufferBase {
protected:
	static constexpr size_t MARK_MAX = std::numeric_limits<size_t>::max();


    uint8_t* array_ 	= nullptr;
    size_t length_	= 0;
    size_t limit_ 	= 0;
    size_t pos_ 	= 0;
    size_t mark_ 	= MARK_MAX;

    bool owner_;

    ValueCodec<uint64_t>    uvlen_codec_;
    ValueCodec<int64_t>     vlen_codec_;
    ValueCodec<U8String>    string_codec_;
    ValueCodec<Bytes>       bytes_codec_;

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

    IOBufferBase(uint8_t* data, size_t length):
        array_(data), length_(length), limit_(length), owner_(false)
    {}

    IOBufferBase(uint8_t* data, size_t pos, size_t length):
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
    

    uint8_t* array() {
        return array_;
    }

    const uint8_t* array() const {
        return array_;
    }

    uint8_t* ptr() {
        return array_ + pos_;
    }

    const uint8_t* ptr() const {
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Supplied value of position exceeds limit: {} {}", v, limit_));
        }
    }

    void skip(size_t len)
    {
    	if (pos_ + len <= limit_)
    	{
    		pos_ += len;
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Supplied value of len exceeds limit: {} {} {}", len, pos_, limit_));
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Supplied value of limit exceeds capacity: {} {}", value, length_));
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
            MMA1_THROW(Exception()) << WhatInfo("IObuffer is not marked");
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

    bool put(int8_t v)
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

    int8_t getByte()
    {
        assertRange(1, "getByte()");
        return (int8_t)array_[pos_++];
    }

    bool put(uint8_t v)
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

    uint8_t getUByte()
    {
        assertRange(1, "getByte()");
        return (uint8_t)array_[pos_++];
    }

    bool put(char v)
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

    uint8_t getChar()
    {
        assertRange(1, "getChar()");
        return (char)array_[pos_++];
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

    bool put(const U8String& str)
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

    U8String getString()
    {
        U8String str;
        pos_ += string_codec_.decode(array_, str, pos_);
        return str;
    }

    size_t length(int64_t val) const
    {
        return vlen_codec_.length(val);
    }

    size_t ulength(uint64_t val) const
    {
        return uvlen_codec_.length(val);
    }

    bool putVLen(int64_t val)
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

    bool putUVLen(uint64_t val)
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


    int64_t getVLen()
    {
        int64_t len = 0;
        pos_ += vlen_codec_.decode(array_, len, pos_);
        return len;
    }

    int64_t getUVLen()
    {
        uint64_t len = 0;
        pos_ += uvlen_codec_.decode(array_, len, pos_);
        return len;
    }

    bool put(const uint8_t* data, size_t length)
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
    
    bool put_(const uint8_t* data, size_t length)
    {
        std::memcpy(array_ + pos_, data, length);
        pos_ += length;
        return true;
    }

    void get(uint8_t* data, size_t length)
    {
        assertRange(length, "get(uint8_t*, size_t)");
        //CopyBuffer(array_ + pos_, data, length);
        std::memcpy(data, array_ + pos_, length);
        pos_ += length;
    }
    
    void get_(uint8_t* data, size_t length)
    {
        std::memcpy(data, array_ + pos_, length);
        pos_ += length;
    }


    bool put(const ValuePtrT1<uint8_t>& value)
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


    template <int32_t Symbols>
    static constexpr int64_t getMaxSymbolsRunLength()
    {
        return MaxRLERunLength;
    }

    template <int32_t Symbols>
    bool putSymbolsRun(int32_t symbol, uint64_t length)
    {
        if (length <= getMaxSymbolsRunLength<Symbols>())
        {
            uint64_t value = memoria::v1::rleseq::EncodeRun<Symbols, MaxRLERunLength>(symbol, length);
            return putUVLen(value);
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Max symbols run length of {} exceeds {}", length, getMaxSymbolsRunLength<Symbols>()));
        }
    }


    template <int32_t Symbols>
    void updateSymbolsRun(size_t pos, int32_t symbol, uint64_t length)
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Max symbols run length of {} exceeds {}", length, getMaxSymbolsRunLength<Symbols>()));
        }
    }


    template <int32_t Symbols>
    memoria::v1::rleseq::RLESymbolsRun getSymbolsRun()
    {
        uint64_t value = getUVLen();
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

            free_system(array_);

            array_  = new_array;
            length_ = new_length;
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("IOBuffer can't enlarge alien data buffer");
        }
    }

    void assertRange(size_t window, const char* op_type)
    {
        if (pos_ + window <= limit_)
        {
            return;
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"IOBuffer:: {} is out of bounds: {} {} {}", op_type, pos_, window, limit_));
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"IOBuffer has no enough free space: {} {}", length, available));
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
            free_system(array_);
        }
    }

    static uint8_t* allocate(size_t length)
    {
        auto ptr = allocate_system<uint8_t>(length);

        if (ptr)
        {
            return ptr.release();
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Can't allocate IOBuffer");
        }
    }
};





}
}
