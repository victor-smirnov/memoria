
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



/**
 * Basic bitmap tools. Get/Set bit and bit groups. Copy/Shift bits in a Buffer.
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

	ID 		ids_[Size];
	Object 	objects_[Size];

public:
	StaticPool()
	{
		for (Int c = 0; c < Size; c++)
		{
			ids_[c] = 0;
		}
	}

	StaticPool(const MyType& other) {}

	MyType& operator=(const MyType& other) {
		return *this;
	}

	Object* Get(const ID& id)
	{
		for (Int c = 0; c < Size; c++)
		{
			if (ids_[c] == id)
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
			ids_[idx] = id;
			objects_[idx].init();
			return &objects_[idx];
		}
		else {
			throw new MemoriaException(MEMORIA_SOURCE, "StaticPool is full");
		}
	}

	void Release(const ID& id)
	{
		for (Int c = 0; c < Size; c++)
		{
			if (ids_[c] == id)
			{
				ids_[c] = ID(0);
				return;
			}
		}

		throw new MemoriaException(MEMORIA_SOURCE, "ID is not known in this StaticPool");
	}

	Int GetUsage() {
		return Size - GetCapacity();
	}

	Int GetCapacity()
	{
		Int cnt = 0;
		for (Int c = 0; c < Size; c++)
		{
			if (ids_[c] == ID(0))
			{
				cnt++;
			}
		}

		return cnt;
	}

	void Clear() {
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
			if (ids_[c] == 0)
			{
				return c;
			}
		}

		return Size;
	}
};


}

#endif
