
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_ACCESSORS_HPP_
#define MEMORIA_CORE_TOOLS_ACCESSORS_HPP_

#include <memoria/core/tools/bitmap.hpp>

#include <functional>

namespace memoria {

using namespace std;

template <typename T, typename V, Int BitsPerElement> class BitmapAccessor;

template <typename T, typename V, Int BitsPerElement>
class BitmapAccessor<T*, V, BitsPerElement> {
	T* values_;
	Int idx_;

public:
	BitmapAccessor(T* values, Int idx): values_(values), idx_(idx) {}

	V value() const {
		return GetBits(values_, idx_, BitsPerElement);
	}

	operator V() const {
		return value();
	}

	V operator=(const V& value)
	{
		MEMORIA_ASSERT(value, <=, (Int)(static_cast<UInt>(-1) >> (TypeBitsize<UInt>() - BitsPerElement)));
		SetBits(values_, idx_ * BitsPerElement, value, BitsPerElement);
		return value;
	}
};

template <typename T, typename V, Int BitsPerElement>
class BitmapAccessor<const T*, V, BitsPerElement> {
	const T* values_;
	Int idx_;
public:
	BitmapAccessor(const T* values, Int idx): values_(values), idx_(idx) {}

	V value() const {
		return GetBits(values_, idx_ * BitsPerElement, BitsPerElement);
	}

	operator V() const {
		return value();
	}
};



template <typename V>
class FnAccessor {
public:

	typedef function <void (const V&)> 	Setter;
	typedef function <V ()>				Getter;

private:
	Getter getter_fn_;
	Setter setter_fn_;

public:
	FnAccessor(Getter getter, Setter setter): getter_fn_(getter), setter_fn_(setter) {}

	V value() const {
		return getter_fn_();
	}

	operator V() const {
		return value();
	}

	V operator=(const V& value)
	{
		setter_fn_(value);
		return value;
	}
};


template <typename V>
class VLEFnAccessor: public FnAccessor<V> {

	typedef FnAccessor<V> Base;

public:
	typedef function <size_t ()>				LengthGetter;

private:

	LengthGetter length_getter_;

public:
	VLEFnAccessor(typename Base::Getter getter, typename Base::Setter setter, LengthGetter length_getter):
		Base(getter, setter), length_getter_(length_getter)
	{}

	size_t length() const {
		return length_getter_();
	}
};


template <typename V>
class ConstFnAccessor {
public:

	typedef function <V ()>				Getter;

private:
	Getter getter_fn_;


public:
	ConstFnAccessor(Getter getter): getter_fn_(getter) {}

	V value() const {
		return getter_fn_();
	}

	operator V() const {
		return value();
	}
};



template <typename V>
class ConstVLEFnAccessor: public ConstFnAccessor<V> {

	typedef FnAccessor<V> Base;

public:
	typedef function <size_t ()>				LengthGetter;

private:

	LengthGetter length_getter_;

public:
	ConstVLEFnAccessor(typename Base::Getter getter, LengthGetter length_getter):
		Base(getter), length_getter_(length_getter)
	{}

	size_t length() const {
		return length_getter_();
	}
};



}


#endif
