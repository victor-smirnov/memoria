
// Copyright Victor Smirnov, Ivan Yurchenko 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_VECTOR_HPP
#define	_MEMORIA_CORE_TOOLS_VECTOR_HPP


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <algorithm>

namespace memoria    {
namespace core       {


struct EmptyValueFunctor {

	template <typename Value>
	void operator()(Value&) {}
};


struct NullPtrFunctor {

	template <typename Value>
	void operator()(Value& val)
	{
		val = NULL;
	}
};

template <typename Value, Int Size = 16, typename ClearingFunctor = EmptyValueFunctor>
class FixedVector {

	typedef FixedVector<Value, Size, ClearingFunctor> MyType;

	Int 	size_;
	Value 	values_[Size];

public:
	typedef Value Element;

	FixedVector(): size_(0) {}

	FixedVector(Int size): size_(size)
	{
		for (Int c = 0; c < Size; c++)
		{
			functor(values_[c]);
		}
	}

	FixedVector(const MyType& other)
	{
		size_ = other.size_;

		for (Int c = 0; c < Size; c++)
		{
			values_[c] = other.values_[c];
		}
	}

	FixedVector(MyType&& other)
	{
		size_ = other.size_;

		for (Int c = 0; c < Size; c++)
		{
			values_[c] = std::move(other.values_[c]);
		}
	}

	MyType& operator=(const MyType& other)
	{
		size_ = other.size_;

		for (Int c = 0; c < size_; c++)
		{
			values_[c] = other.values_[c];
		}

		ClearingFunctor functor;

		for (Int c = size_; c < Size; c++)
		{
			functor(values_[c]);
		}

		return *this;
	}

	MyType& operator=(MyType&& other)
	{
		size_ = other.size_;

		for (Int c = 0; c < size_; c++)
		{
			values_[c] = std::move(other.values_[c]);
		}

		ClearingFunctor functor;

		for (Int c = size_; c < Size; c++)
		{
			functor(values_[c]);
		}

		return *this;
	}

	const Value& operator[](Int idx) const {
		return values_[idx];
	}

	Value& operator[](Int idx) {
		return values_[idx];
	}

	Int getSize() const {
		return size_;
	}

	void resize(Int size)
	{
		size_ = size;
	}

	static Int getMaxSize() {
		return Size;
	}

	void insert(Int idx, const Value& value)
	{
		for (Int c = size_; c > idx; c--)
		{
			values_[c] = values_[c - 1];
		}

		values_[idx] = value;
		size_++;
	}

	void append(const Value& value)
	{
		values_[size_++] = value;
	}

	void remove(Int idx)
	{
		for (Int c = idx; c < size_; c++)
		{
			values_[c] = values_[c + 1];
		}

		ClearingFunctor functor;

		size_--;

		functor(values_[size_]);
	}

	void removeLast()
	{
		remove(getSize() - 1);
	}

	void clear()
	{
		ClearingFunctor functor;

		for (Int c = 0; c < size_; c++)
		{
			functor(values_[c]);
		}

		size_ = 0;
	}
};

}
}
#endif

