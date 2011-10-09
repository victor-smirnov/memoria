
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CORE_TOOLS_PMAP_VLEN_HPP_
#define _MEMORIA_CORE_TOOLS_PMAP_VLEN_HPP_

#include <memoria/core/types/types.hpp>
#include <iostream>

namespace memoria 	{

template <typename T>
class Ptr {
	T* ptr_;
public:
	// TODO: must throw if ptr is null
	Ptr(void* ptr): ptr_(static_cast<T*>(ptr)){}
	const T& value() const {
		return *ptr_;
	}

	T& value() {
		return *ptr_;
	}
};



template <typename T>
void Copy(const T* from, T* to, Int Size) {
	for (int c = 0;c < Size; c++)
	{
		to[c] = from[c];
	}
}

template <typename T>
bool Compare(const T* from, const T* to, Int Size) {
	for (int c = 0;c < Size; c++)
	{
		if (to[c] != from[c]) {
			return false;
		}
	}
	return true;
}



template <typename T>
class VLengthArray {
	typedef VLengthArray<T> Me;
	typedef VLengthArray<const T> ConstMe;

	UInt length_;
	T* data_;
public:
	typedef T AtomType;
	static const UInt ATOM_SIZE = sizeof(T);

	// TODO: must throw if ptr is null
	VLengthArray(UInt length, void* ptr): length_(length / sizeof(T)), data_(static_cast<T*>(ptr)){}

	const T& operator[](UInt idx) const {
		return data_[idx];
	}

	T& operator[](UInt idx) {
		return data_[idx];
	}

	const UInt length() const {
		return length_;
	}

	const Me& operator=(const VLengthArray<T>& other)
	{
		if (length_ == other.length_)
		{
			Copy(other.data_, data_, length_);
		}
		else {
			// TODO: throw
		}

		return *this;
	}

	const Me& operator=(const ConstMe& other)
	{
		if (length_ == other.length_)
		{
			Copy(other.ptr(), data_, length_);
		}
		else {
			// TODO: throw
		}

		return *this;
	}

	bool operator==(const Me&other) const
	{
		if (length_ == other.length_)
		{
			return Compare(data_, other.data_, length_);
		}
		else {
			//TODO: throw
			return false;
		}
	}

	bool operator==(const ConstMe& other) const
	{
		if (length_ == other.length())
		{
			return Compare(data_, other.ptr(), length_);
		}
		else {
			//TODO: throw
			return false;
		}
	}

	bool operator!=(const Me& other) const
	{
		return !operator==(other);
	}

	bool operator!=(const ConstMe& other) const
	{
		return !operator==(other);
	}

	UByte* ptr() {
		return reinterpret_cast<UByte*>(data_);
	}

	const UByte* ptr() const {
		return reinterpret_cast<const UByte*>(data_);
	}
};

template <typename T>
class VLengthArray<const T> {
	typedef VLengthArray<const T> Me;
	typedef VLengthArray<T> NonConstMe;

	UInt length_;
	const T* data_;
public:
	typedef T AtomType;
	static const UInt ATOM_SIZE = sizeof(T);

	// TODO: must throw if ptr is null
	VLengthArray(UInt length, const void* ptr): length_(length / sizeof(T)), data_(static_cast<const T*>(ptr)){}

	const T& operator[](UInt idx) const {
		return data_[idx];
	}

	const UInt length() const {
		return length_;
	}

	bool operator==(const Me&other) const
	{
		if (length_ == other.length_)
		{
			return Compare(data_, other.data_, length_);
		}
		else {
			//TODO: throw
			return false;
		}
	}

	bool operator==(const NonConstMe&other) const
	{
		if (length_ == other.length_)
		{
			return Compare(data_, other.ptr(), length_);
		}
		else {
			//TODO: throw
			return false;
		}
	}

	bool operator!=(const Me&other) const
	{
		return !operator==(other);
	}

	bool operator!=(const NonConstMe&other) const
	{
		return !operator==(other);
	}

	const UByte* ptr() const {
		return reinterpret_cast<const UByte*>(data_);
	}
};



}

namespace std {

using namespace memoria;

template <typename T>
std::ostream& operator<< (std::ostream& out, const VLengthArray<T>& value) {
	const char* letters[] = {"0000", "1000", "0100", "1100",
							 "0010", "1010", "0110", "1110",
							 "0001", "1001", "0101", "1101",
							 "0011", "1011", "0111", "1111"};


	for (int c = 0; c < value.length() * sizeof(T); c++)
	{
		UByte v0 = *(value.ptr() + c);
		out<<letters[v0 & 0xF];
		out<<letters[(v0 & 0xF0) >> 4];
	}

	return out;
}

}

#endif
