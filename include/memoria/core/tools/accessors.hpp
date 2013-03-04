
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_ACCESSORS_HPP_
#define MEMORIA_CORE_TOOLS_ACCESSORS_HPP_

#include <memoria/core/tools/bitmap.hpp>

namespace memoria {

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
		SetBits(values_, idx_, value, BitsPerElement);
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
		return GetBits(values_, idx_, BitsPerElement);
	}

	operator V() const {
		return value();
	}
};


}


#endif
