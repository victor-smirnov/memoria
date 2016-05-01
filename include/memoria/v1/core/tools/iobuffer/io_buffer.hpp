
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
#include <memoria/v1/core/tools/bignum/int64_codec.hpp>

#include <malloc.h>

namespace memoria {
namespace v1 {

class IOBuffer {
	UByte* array_;
	size_t length_;
	size_t pos_ = 0;
	bool owner_;

	ValueCodec<int64_t> vlen_codec_;
	ValueCodec<String>  string_codec_;
	ValueCodec<Bytes>   bytes_codec_;

public:
	IOBuffer():
		array_(nullptr), length_(0), owner_(false)
	{}

	IOBuffer(size_t length):
		array_(allocate(length)), length_(length), owner_(true)
	{}

	IOBuffer(UByte* data, size_t length):
		array_(data), length_(length), owner_(false)
	{}

	IOBuffer(const IOBuffer&) = delete;

	IOBuffer(IOBuffer&& other):
		array_(other.array_), length_(other.length_), pos_(other.pos_), owner_(true)
	{
		other.array_ = nullptr;
		other.owner_ = false;
	}

	~IOBuffer() {
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

	void put(Byte v)
	{
		assertRange(1, "put(Byte)");
		array_[pos_++] = v;
	}

	Byte getByte()
	{
		assertRange(1, "getByte()");
		return (Byte)array_[pos_++];
	}

	void put(UByte v)
	{
		assertRange(1, "put(UByte)");
		array_[pos_++] = v;
	}

	UByte getUByte()
	{
		assertRange(1, "getByte()");
		return (UByte)array_[pos_++];
	}

	void put(Char v)
	{
		assertRange(1, "put(Char)");
		array_[pos_++] = v;
	}

	UByte getChar()
	{
		assertRange(1, "getChar()");
		return (Char)array_[pos_++];
	}


	void put(Short v)
	{
		assertRange(2, "put(Short)");
		array_[pos_++] = v & 0xFF;
		array_[pos_++] = (v >> 8) & 0xFF;
	}

	Short getShort()
	{
		assertRange(2, "getShort()");
		Short v = 0;

		v = array_[pos_++];
		v |= ((Short)array_[pos_++]) << 8;

		return v;
	}


	void put(UShort v)
	{
		assertRange(2, "put(UShort)");
		array_[pos_++] = v & 0xFF;
		array_[pos_++] = (v >> 8) & 0xFF;
	}

	UShort getUShort()
	{
		assertRange(2, "getShort()");
		Short v = 0;

		v = array_[pos_++];
		v |= ((UShort)array_[pos_++]) << 8;

		return v;
	}

	void put(Int v)
	{
		assertRange(4, "put(Int)");
		array_[pos_++] = v & 0xFF;
		array_[pos_++] = (v >> 8) & 0xFF;
		array_[pos_++] = (v >> 16) & 0xFF;
		array_[pos_++] = (v >> 24) & 0xFF;
	}

	Int getInt()
	{
		assertRange(4, "getInt()");
		Int v = 0;

		v = array_[pos_++];
		v |= ((Int)array_[pos_++]) << 8;
		v |= ((Int)array_[pos_++]) << 14;
		v |= ((Int)array_[pos_++]) << 24;

		return v;
	}


	void put(UInt v)
	{
		assertRange(4, "put(UInt)");
		array_[pos_++] = v & 0xFF;
		array_[pos_++] = (v >> 8) & 0xFF;
		array_[pos_++] = (v >> 16) & 0xFF;
		array_[pos_++] = (v >> 24) & 0xFF;
	}

	UInt getUInt()
	{
		assertRange(4, "getUInt()");
		UInt v = 0;

		v = array_[pos_++];
		v |= ((UInt)array_[pos_++]) << 8;
		v |= ((UInt)array_[pos_++]) << 14;
		v |= ((UInt)array_[pos_++]) << 24;

		return v;
	}

	void put(BigInt v)
	{
		assertRange(8, "put(BigInt)");
		array_[pos_++] = v & 0xFF;
		array_[pos_++] = (v >> 8) & 0xFF;
		array_[pos_++] = (v >> 16) & 0xFF;
		array_[pos_++] = (v >> 24) & 0xFF;
		array_[pos_++] = (v >> 32) & 0xFF;
		array_[pos_++] = (v >> 40) & 0xFF;
		array_[pos_++] = (v >> 48) & 0xFF;
		array_[pos_++] = (v >> 56) & 0xFF;
	}

	BigInt getBigInt()
	{
		assertRange(8, "getBigInt()");
		Int v = 0;

		v = array_[pos_++];
		v |= ((BigInt)array_[pos_++]) << 8;
		v |= ((BigInt)array_[pos_++]) << 14;
		v |= ((BigInt)array_[pos_++]) << 24;
		v |= ((BigInt)array_[pos_++]) << 32;
		v |= ((BigInt)array_[pos_++]) << 40;
		v |= ((BigInt)array_[pos_++]) << 48;
		v |= ((BigInt)array_[pos_++]) << 56;

		return v;
	}

	void put(UBigInt v)
	{
		assertRange(8, "put(UBigInt)");
		array_[pos_++] = v & 0xFF;
		array_[pos_++] = (v >> 8) & 0xFF;
		array_[pos_++] = (v >> 16) & 0xFF;
		array_[pos_++] = (v >> 24) & 0xFF;
		array_[pos_++] = (v >> 32) & 0xFF;
		array_[pos_++] = (v >> 40) & 0xFF;
		array_[pos_++] = (v >> 48) & 0xFF;
		array_[pos_++] = (v >> 56) & 0xFF;
	}

	UBigInt getUBigInt()
	{
		assertRange(8, "getUBigInt()");
		Int v = 0;

		v = array_[pos_++];
		v |= ((UBigInt)array_[pos_++]) << 8;
		v |= ((UBigInt)array_[pos_++]) << 14;
		v |= ((UBigInt)array_[pos_++]) << 24;
		v |= ((UBigInt)array_[pos_++]) << 32;
		v |= ((UBigInt)array_[pos_++]) << 40;
		v |= ((UBigInt)array_[pos_++]) << 48;
		v |= ((UBigInt)array_[pos_++]) << 56;

		return v;
	}

	void put(float f)
	{
		assertRange(4, "put(float)");

		Int v = static_cast<Int>(f);

		array_[pos_++] = v & 0xFF;
		array_[pos_++] = (v >> 8) & 0xFF;
		array_[pos_++] = (v >> 16) & 0xFF;
		array_[pos_++] = (v >> 24) & 0xFF;
	}

	float getFloat()
	{
		assertRange(4, "getFloat()");
		Int v = 0;

		v = array_[pos_++];
		v |= ((Int)array_[pos_++]) << 8;
		v |= ((Int)array_[pos_++]) << 14;
		v |= ((Int)array_[pos_++]) << 24;

		return static_cast<float>(v);
	}


	void put(double f)
	{
		assertRange(8, "put(double)");

		BigInt v = static_cast<BigInt>(f);

		array_[pos_++] = v & 0xFF;
		array_[pos_++] = (v >> 8) & 0xFF;
		array_[pos_++] = (v >> 16) & 0xFF;
		array_[pos_++] = (v >> 24) & 0xFF;
		array_[pos_++] = (v >> 32) & 0xFF;
		array_[pos_++] = (v >> 40) & 0xFF;
		array_[pos_++] = (v >> 48) & 0xFF;
		array_[pos_++] = (v >> 56) & 0xFF;
	}

	double getDouble()
	{
		assertRange(8, "getDouble()");
		BigInt v = 0;

		v = array_[pos_++];
		v |= ((BigInt)array_[pos_++]) << 8;
		v |= ((BigInt)array_[pos_++]) << 14;
		v |= ((BigInt)array_[pos_++]) << 24;
		v |= ((BigInt)array_[pos_++]) << 32;
		v |= ((BigInt)array_[pos_++]) << 40;
		v |= ((BigInt)array_[pos_++]) << 48;
		v |= ((BigInt)array_[pos_++]) << 56;

		return static_cast<double>(v);
	}

	void put(bool v)
	{
		assertRange(1, "put(bool)");
		array_[pos_++] = v;
	}

	bool getBool()
	{
		assertRange(1, "getBool()");
		return (bool)array_[pos_++];
	}

	void put(const Bytes& bytes)
	{
		assertRange(bytes_codec_.length(bytes), "put(Bytes)");
		pos_ += bytes_codec_.encode(array_, bytes, pos_);
	}

	Bytes getBytes()
	{
		int64_t len = 0;
		pos_ += vlen_codec_.encode(array_, len, pos_);

		Bytes bytes(array_ + pos_, len, false);

		pos_ += bytes.length();

		return bytes;
	}

	void put(const String& str)
	{
		assertRange(string_codec_.length(str), "put(String)");
		pos_ += string_codec_.encode(array_, str, pos_);
	}

	String getString()
	{
		String str;
		pos_ += string_codec_.decode(array_, str, pos_);
		return str;
	}

	size_t length(int64_t val) const
	{
		return vlen_codec_.length(val);
	}

	void putVLen(int64_t val)
	{
		size_t len = vlen_codec_.length(val);
		assertRange(len, "putVLen(int64_t)");

		pos_ += vlen_codec_.encode(array_, val, pos_);
	}

	int64_t getVLen()
	{
		int64_t len = 0;
		pos_ += vlen_codec_.encode(array_, len, pos_);
		return len;
	}

	void put(const UByte* data, size_t length)
	{
		assertRange(length, "put(UByte*, size_t)");
		CopyBuffer(data, array_ + pos_, length);
		pos_ += length;
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

private:
	void assertRange(size_t window, const char* op_type)
	{
		if (pos_ + window > length_)
		{
			throw Exception(MA_SRC, SBuf() << "IOBuffer::" << op_type << " is out of bounds: " << pos_ << " " << window << " " << length_);
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
