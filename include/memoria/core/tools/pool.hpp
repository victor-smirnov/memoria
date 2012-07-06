
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



/**
 * Basic bitmap tools. get/set bit and bit groups. Copy/Shift bits in a Buffer.
 *
 * Buffer must have Long type declarator and [] overloaded operator.
 */

#ifndef _MEMORIA_CORE_TOOLS_POOL_H
#define _MEMORIA_CORE_TOOLS_POOL_H

#include <memoria/core/tools/buffer.hpp>


namespace memoria {


template <typename ID, typename Object, Int Size = 64>
class StaticPool {
	typedef StaticPool<ID, Object, Size> MyType;
	typedef ID T;

	T 		ids_[Size];
	Object 	objects_[Size];
	UByte	idxs_[Size];
	Int 	size_;
	Int 	Max;

public:
	StaticPool(): size_(0), Max(0)
	{
		for (Int c = 0; c < Size; c++)
		{
			ids_[c] = 0;
		}
	}

	StaticPool(const MyType& other): size_(0), Max(0) {}

	MyType& operator=(const MyType& other) {
		return *this;
	}

	Object* get(const ID& id)
	{
		const T idv = id.value();

		for (Int c = 0; c < Max; c++)
		{
			if (ids_[c] == idv)
			{
				return &objects_[c];
			}
		}

		return NULL;
	}

	Object* Allocate(const ID& id)
	{
		Int idx = SelectFirst0Idx();
		if (idx < Size)
		{
			size_++;

			if (size_ > Max && Max < Size) Max = size_;

			ids_[idx] = id;
			objects_[idx].init();
			return &objects_[idx];
		}
		else {
			throw new Exception(MEMORIA_SOURCE, "StaticPool is full");
		}
	}

	void Release(const ID& id)
	{
		for (Int c = 0; c < Size; c++)
		{
			if (ids_[c] == id)
			{
				size_--;
				ids_[c] = 0;
				return;
			}
		}

		throw new Exception(MEMORIA_SOURCE, "ID is not known in this StaticPool");
	}

	Int getMax() {
		return Max;
	}

	Int getUsage() {
		return Size - getCapacity();
	}

	Int getCapacity()
	{
		Int cnt = 0;
		for (Int c = 0; c < Size; c++)
		{
			if (ids_[c] == 0)
			{
				cnt++;
			}
		}

		return cnt;
	}

	void clear() {
		for (Int c = 0; c < Size; c++)
		{
			ids_[c] = ID(0);
		}
	}

private:
	Int SelectFirst0Idx()
	{
		const ID EMPTY(0);
		for (Int c = 0; c < Size; c++)
		{
			if (ids_[c] == EMPTY)
			{
				return c;
			}
		}

		return Size;
	}
};


}

#endif
