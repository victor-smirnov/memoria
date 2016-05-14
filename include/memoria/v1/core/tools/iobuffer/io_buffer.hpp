
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
#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/uuid.hpp>


#include <limits>

#include <malloc.h>

namespace memoria {
namespace v1 {

class IOBuffer {
	UByte* array_ = nullptr;
	size_t length_ = 0;
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


	bool put(Short v)
	{
		if (has_capacity(2))
		{
			array_[pos_++] = v & 0xFF;
			array_[pos_++] = (v >> 8) & 0xFF;
			return true;
		}
		else {
			return false;
		}
	}

	Short getShort()
	{
		assertRange(2, "getShort()");
		Short v = 0;

		v = array_[pos_++];
		v |= ((Short)array_[pos_++]) << 8;

		return v;
	}


	bool put(UShort v)
	{
		if (has_capacity(2))
		{
			array_[pos_++] = v & 0xFF;
			array_[pos_++] = (v >> 8) & 0xFF;
			return true;
		}
		else {
			return false;
		}
	}

	UShort getUShort()
	{
		assertRange(2, "getShort()");
		Short v = 0;

		v = array_[pos_++];
		v |= ((UShort)array_[pos_++]) << 8;

		return v;
	}

	bool put(Int v)
	{
		if (has_capacity(4))
		{
			array_[pos_++] = v & 0xFF;
			array_[pos_++] = (v >> 8) & 0xFF;
			array_[pos_++] = (v >> 16) & 0xFF;
			array_[pos_++] = (v >> 24) & 0xFF;
			return true;
		}
		else {
			return false;
		}
	}

	Int getInt()
	{
		assertRange(4, "getInt()");
		Int v = 0;

		v = array_[pos_++];
		v |= ((Int)array_[pos_++]) << 8;
		v |= ((Int)array_[pos_++]) << 16;
		v |= ((Int)array_[pos_++]) << 24;

		return v;
	}


	bool put(UInt v)
	{
		if (has_capacity(4))
		{
			array_[pos_++] = v & 0xFF;
			array_[pos_++] = (v >> 8) & 0xFF;
			array_[pos_++] = (v >> 16) & 0xFF;
			array_[pos_++] = (v >> 24) & 0xFF;
			return true;
		}
		else {
			return false;
		}
	}

	UInt getUInt()
	{
		assertRange(4, "getUInt()");
		UInt v = 0;

		v = array_[pos_++];
		v |= ((UInt)array_[pos_++]) << 8;
		v |= ((UInt)array_[pos_++]) << 16;
		v |= ((UInt)array_[pos_++]) << 24;

		return v;
	}

	bool put(BigInt v)
	{
		if (has_capacity(8))
		{
			array_[pos_++] = v & 0xFF;
			array_[pos_++] = (v >> 8) & 0xFF;
			array_[pos_++] = (v >> 16) & 0xFF;
			array_[pos_++] = (v >> 24) & 0xFF;
			array_[pos_++] = (v >> 32) & 0xFF;
			array_[pos_++] = (v >> 40) & 0xFF;
			array_[pos_++] = (v >> 48) & 0xFF;
			array_[pos_++] = (v >> 56) & 0xFF;
			return true;
		}
		else {
			return false;
		}
	}

	BigInt getBigInt()
	{
		assertRange(8, "getBigInt()");
		BigInt v = 0;

		v = array_[pos_++];
		v |= ((BigInt)array_[pos_++]) << 8;
		v |= ((BigInt)array_[pos_++]) << 16;
		v |= ((BigInt)array_[pos_++]) << 24;
		v |= ((BigInt)array_[pos_++]) << 32;
		v |= ((BigInt)array_[pos_++]) << 40;
		v |= ((BigInt)array_[pos_++]) << 48;
		v |= ((BigInt)array_[pos_++]) << 56;

		return v;
	}

	bool put(UBigInt v)
	{
		if (has_capacity(8))
		{
//			*T2T<UBigInt*>(array_ + pos_) = __builtin_bswap64(v);
//			pos_ += 8;

			array_[pos_] = v & 0xFF;
			array_[pos_+1] = (v >> 8) & 0xFF;
			array_[pos_+2] = (v >> 16) & 0xFF;
			array_[pos_+3] = (v >> 24) & 0xFF;
			array_[pos_+4] = (v >> 32) & 0xFF;
			array_[pos_+5] = (v >> 40) & 0xFF;
			array_[pos_+6] = (v >> 48) & 0xFF;
			array_[pos_+7] = (v >> 56) & 0xFF;

			pos_ += 8;

			return true;
		}
		else {
			return false;
		}
	}

	UBigInt getUBigInt()
	{
		assertRange(8, "getUBigInt()");
//		UBigInt v = __builtin_bswap64(*T2T<const UBigInt*>(array_ + pos_));
//		pos_ += 8;


		UBigInt v = 0;
		v = array_[pos_];
		v |= ((UBigInt)array_[pos_+1]) << 8;
		v |= ((UBigInt)array_[pos_+2]) << 16;
		v |= ((UBigInt)array_[pos_+3]) << 24;
		v |= ((UBigInt)array_[pos_+4]) << 32;
		v |= ((UBigInt)array_[pos_+5]) << 40;
		v |= ((UBigInt)array_[pos_+6]) << 48;
		v |= ((UBigInt)array_[pos_+7]) << 56;
		pos_ += 8;

		return v;
	}

	bool put(float f)
	{
		if (has_capacity(4))
		{
			Int v = static_cast<Int>(f);

			array_[pos_++] = v & 0xFF;
			array_[pos_++] = (v >> 8) & 0xFF;
			array_[pos_++] = (v >> 16) & 0xFF;
			array_[pos_++] = (v >> 24) & 0xFF;
			return true;
		}
		else {
			return false;
		}
	}

	float getFloat()
	{
		assertRange(4, "getFloat()");
		Int v = 0;

		v = array_[pos_++];
		v |= ((Int)array_[pos_++]) << 8;
		v |= ((Int)array_[pos_++]) << 16;
		v |= ((Int)array_[pos_++]) << 24;

		return static_cast<float>(v);
	}


	bool put(double f)
	{
		if (has_capacity(8))
		{
			BigInt v = static_cast<BigInt>(f);

			array_[pos_++] = v & 0xFF;
			array_[pos_++] = (v >> 8) & 0xFF;
			array_[pos_++] = (v >> 16) & 0xFF;
			array_[pos_++] = (v >> 24) & 0xFF;
			array_[pos_++] = (v >> 32) & 0xFF;
			array_[pos_++] = (v >> 40) & 0xFF;
			array_[pos_++] = (v >> 48) & 0xFF;
			array_[pos_++] = (v >> 56) & 0xFF;
			return true;
		}
		else {
			return false;
		}
	}

	double getDouble()
	{
		assertRange(8, "getDouble()");
		BigInt v = 0;

		v = array_[pos_++];
		v |= ((BigInt)array_[pos_++]) << 8;
		v |= ((BigInt)array_[pos_++]) << 16;
		v |= ((BigInt)array_[pos_++]) << 24;
		v |= ((BigInt)array_[pos_++]) << 32;
		v |= ((BigInt)array_[pos_++]) << 40;
		v |= ((BigInt)array_[pos_++]) << 48;
		v |= ((BigInt)array_[pos_++]) << 56;

		return static_cast<double>(v);
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

	size_t length(int64_t val) const
	{
		return vlen_codec_.length(val);
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

	int64_t getVLen()
	{
		int64_t len = 0;
		pos_ += vlen_codec_.decode(array_, len, pos_);
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
		constexpr Int BitsPerSymbol = NumberOfBits(Symbols);
		return numeric_limits<int64_t>::max() >> BitsPerSymbol;
	}

	template <Int Symbols>
	bool putSymbolsRun(Int symbol, BigInt length)
	{
		constexpr Int BitsPerSymbol 	= NumberOfBits(Symbols);

		if (length <= getMaxSymbolsRunLength<Symbols>())
		{
			BigInt value = symbol | (length << BitsPerSymbol);
			return putVLen(value);
		}
		else {
			throw Exception(MA_SRC, SBuf() << "Max symbols run length of " << length << " exceeds " << getMaxSymbolsRunLength<Symbols>());
		}
	}

	class SymbolsRun {
		Int symbol_;
		BigInt length_;
	public:
		SymbolsRun(Int symbol, BigInt length): symbol_(symbol), length_(length) {}

		Int symbol() const {return symbol_;};
		BigInt length() const {return length_;}
	};

	template <Int Symbols>
	SymbolsRun getSymbolsRun()
	{
		static constexpr BigInt BitsPerSymbol = NumberOfBits(Symbols);
		static constexpr BigInt LevelCodeMask = (1 << BitsPerSymbol) - 1;

		BigInt value = getVLen();

		return SymbolsRun(value & LevelCodeMask, value >> BitsPerSymbol);
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

private:
	void assertRange(size_t window, const char* op_type)
	{
//		if (pos_ + window <= length_)
//		{
//			return;
//		}
//		else {
//			throw Exception(MA_SRC, SBuf() << "IOBuffer::" << op_type << " is out of bounds: " << pos_ << " " << window << " " << length_);
//		}
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



template <typename T>
struct IOBufferAdaptorBase {
	static bool put(IOBuffer& buffer, const T& value) {
		return buffer.put(value);
	}
};

template <>
struct IOBufferAdaptor<Char>: IOBufferAdaptorBase<Char> {
	static Char get(IOBuffer& buffer) {
		return buffer.getChar();
	}
};

template <>
struct IOBufferAdaptor<Byte>: IOBufferAdaptorBase<Byte> {
	static Byte get(IOBuffer& buffer) {
		return buffer.getByte();
	}
};

template <>
struct IOBufferAdaptor<UByte>: IOBufferAdaptorBase<UByte> {
	static UByte get(IOBuffer& buffer) {
		return buffer.getUByte();
	}
};

template <>
struct IOBufferAdaptor<Short>: IOBufferAdaptorBase<Short> {
	static Short get(IOBuffer& buffer) {
		return buffer.getShort();
	}
};

template <>
struct IOBufferAdaptor<UShort>: IOBufferAdaptorBase<UShort> {
	static UShort get(IOBuffer& buffer) {
		return buffer.getUShort();
	}
};

template <>
struct IOBufferAdaptor<Int>: IOBufferAdaptorBase<Int> {
	static Int get(IOBuffer& buffer) {
		return buffer.getInt();
	}
};

template <>
struct IOBufferAdaptor<UInt>: IOBufferAdaptorBase<UInt> {
	static UInt get(IOBuffer& buffer) {
		return buffer.getUInt();
	}
};


template <>
struct IOBufferAdaptor<BigInt>: IOBufferAdaptorBase<BigInt> {
	static BigInt get(IOBuffer& buffer) {
		return buffer.getBigInt();
	}
};

template <>
struct IOBufferAdaptor<UBigInt>: IOBufferAdaptorBase<UBigInt> {
	static UBigInt get(IOBuffer& buffer) {
		return buffer.getUBigInt();
	}
};

template <>
struct IOBufferAdaptor<float>: IOBufferAdaptorBase<float> {
	static float get(IOBuffer& buffer) {
		return buffer.getFloat();
	}
};


template <>
struct IOBufferAdaptor<double>: IOBufferAdaptorBase<double> {
	static double get(IOBuffer& buffer) {
		return buffer.getDouble();
	}
};

template <>
struct IOBufferAdaptor<bool>: IOBufferAdaptorBase<bool> {
	static double get(IOBuffer& buffer) {
		return buffer.getBool();
	}
};

template <>
struct IOBufferAdaptor<Bytes>: IOBufferAdaptorBase<Bytes> {
	static Bytes get(IOBuffer& buffer) {
		return buffer.getBytes();
	}
};

template <>
struct IOBufferAdaptor<String>: IOBufferAdaptorBase<String> {
	static String get(IOBuffer& buffer) {
		return buffer.getString();
	}
};

template <>
struct IOBufferAdaptor<UUID> {

	static bool put(IOBuffer& buffer, const UUID& value)
	{
		if (buffer.put(value.hi()))
		{
			if (buffer.put(value.lo())) {
				return true;
			}
		}

		return false;
	}

	static UUID get(IOBuffer& buffer)
	{
		return UUID(buffer.getUBigInt(), buffer.getUBigInt());
	}
};



}
}
