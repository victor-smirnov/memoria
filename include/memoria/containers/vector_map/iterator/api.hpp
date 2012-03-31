
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_VECTOR_MAP_ITERATOR_API_HPP
#define _MEMORIA_MODELS_VECTOR_MAP_ITERATOR_API_HPP

#include <iostream>

#include <memoria/core/types/types.hpp>

#include <memoria/containers/vector_map/names.hpp>
#include <memoria/core/container/iterator.hpp>

namespace memoria    {

using namespace memoria::vector_map;

MEMORIA_ITERATOR_PART_BEGIN(memoria::vector_map::ItrApiName)

typedef Ctr<VectorMapCtrTypes<Types> >						ContainerType;

typedef typename ContainerType::IdxSet::Iterator			IdxSetIterator;
typedef typename ContainerType::ByteArray::Iterator			ByteArrayIterator;

typedef typename ContainerType::IdxSet::Accumulator			IdxSetAccumulator;

typedef typename Types::Profile								Profile;
typedef typename Types::Allocator 							Allocator;
typedef typename Types::Allocator::CtrShared 				CtrShared;
typedef typename Types::Allocator::PageG 					PageG;
typedef typename Allocator::Page							Page;
typedef typename Page::ID									ID;

typedef typename ContainerType::Key							Key;




void Insert(const ArrayData& data)
{
	me()->ba_iter().Insert(data);

	IdxSetAccumulator keys;

	keys.key(1) = data.size();

	me()->is_iter().UpdateUp(keys);
}


BigInt Read(ArrayData& data)
{
	return Read(data, 0, data.size());
}


BigInt Read(ArrayData& data, BigInt start, BigInt length)
{
	BigInt current_pos 	= me()->pos();
	BigInt current_size = me()->size();

	BigInt len = ((length + current_pos) <= current_size) ? length : (current_size - current_pos);

	me()->ba_iter().Read(data, start, len);

	return len;
}

BigInt Skip(BigInt length)
{
	BigInt current 	= me()->pos();

	if (length > 0)
	{
		BigInt current_size = size();
		if (length + current > current_size)
		{
			length = current_size - current;
		}
	}
	else {
		if (length + current < 0)
		{
			length = -current;
		}
	}

	return me()->ba_iter().Skip(length);
}

void Remove()
{
	BigInt data_size = me()->size();
	me()->model().set().RemoveEntry(me()->is_iter());
	me()->ba_iter().Remove(data_size);
}

BigInt size()
{
	return me()->is_iter().GetRawKey(1);
}

BigInt pos()
{
	return me()->ba_iter().pos() - me()->is_iter().prefix(1);
}

BigInt GetKey()
{
	return me()->is_iter().GetKey(0);
}

IdxSetAccumulator GetKeys()
{
	IdxSetAccumulator keys = me()->is_iter().GetRawKeys();

	keys.key(0) += me()->is_iter().prefix(0);

	return keys;
}

bool IsNotEnd()
{
	return me()->is_iter().IsNotEnd();
}

bool IsEnd()
{
	return me()->is_iter().IsEnd();
}

bool Next()
{
	me()->ba_iter().Skip(size() - pos());
	return me()->is_iter().Next();
}


MEMORIA_ITERATOR_PART_END

}


#endif
