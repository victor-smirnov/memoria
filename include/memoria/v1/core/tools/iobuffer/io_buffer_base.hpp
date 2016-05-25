
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
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/bytes/bytes_codec.hpp>
#include <memoria/v1/core/tools/bignum/primitive_codec.hpp>
#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/packed/sseq/rleseq/rleseq_tools.hpp>

#include <limits>

#include <malloc.h>

namespace memoria {
namespace v1 {

template <ByteOrder Endian, MemoryAccess AccessType> class IOBuffer;

class IOBufferBase {
protected:
	UByte* array_ = nullptr;
	size_t length_ = 0;
	size_t pos_ = 0;
	bool owner_;

	ValueCodec<UBigInt> uvlen_codec_;
	ValueCodec<BigInt>  vlen_codec_;
	ValueCodec<String>  string_codec_;
	ValueCodec<Bytes>   bytes_codec_;

public:

	IOBufferBase():
		array_(nullptr), length_(0), owner_(false)
{}

	IOBufferBase(size_t length):
		array_(allocate(length)), length_(length), owner_(true)
	{}

	IOBufferBase(UByte* data, size_t length):
		array_(data), length_(length), owner_(false)
	{}

	IOBufferBase(const IOBufferBase&) = delete;

	IOBufferBase(IOBufferBase&& other):
		array_(other.array_), length_(other.length_), pos_(other.pos_), owner_(true)
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

	void rewind() {
		pos_ = 0;
	}

	size_t pos() const {
		return pos_;
	}

	void pos(size_t v) {
		pos_ = v;
	}

	void skip(size_t len) {
		pos_ += len;
	}

	size_t capacity() const {
		return length_ - pos_;
	}

	bool has_capacity(size_t size) const {
		return capacity() >= size;
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
		if (has_capacity(length))
		{
			CopyBuffer(data, array_ + pos_, length);
			pos_ += length;
			return true;
		}
		else {
			return false;
		}
	}

	void get(UByte* data, size_t length)
	{
		assertRange(length, "get(UByte*, size_t)");
		CopyBuffer(array_ + pos_, data, length);
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
	memoria::v1::rleseq::RLESymbolsRun getSymbolsRun()
	{
		UBigInt value = getUVLen();

		return memoria::v1::rleseq::DecodeRun<Symbols>(value);
	}

	void enlarge(size_t minimal_capacity = 0)
	{
		if (owner_)
		{
			size_t new_length 	= length_ * 2;
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
		if (pos_ + window <= length_)
		{
			return;
		}
		else {
			throw Exception(MA_SRC, SBuf() << "IOBuffer::" << op_type << " is out of bounds: " << pos_ << " " << window << " " << length_);
		}
	}

protected:

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
