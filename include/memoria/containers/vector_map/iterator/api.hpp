
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

typedef typename ContainerType::Idxset::Iterator			IdxsetIterator;
typedef typename ContainerType::ByteArray::Iterator			ByteArrayIterator;

typedef typename ContainerType::Idxset::Accumulator			IdxsetAccumulator;

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

	IdxsetAccumulator keys;

	keys.key(1) = data.size();

	me()->is_iter().UpdateUp(keys);
}

void Update(const ArrayData& data)
{
	BigInt sz = me()->size();

	if (sz > 0)
	{
		BigInt difference = sz - data.size();

		if (difference < 0)
		{
			me()->ba_iter().Update(data);
			me()->ba_iter().Remove(difference);
		}
		else if (difference > 0)
		{
			me()->ba_iter().Update(data, 0, sz);
			me()->ba_iter().Insert(data, sz, difference);
		}
		else {
			me()->ba_iter().Update(data);
		}

		if (difference != 0)
		{
			IdxsetAccumulator keys;
			keys.key(1) = difference;
			me()->is_iter().UpdateUp(keys);
		}
	}
	else {
		me()->Insert(data);
	}

	me()->ba_iter().Skip(-data.size());
}


ArrayData Read()
{
	me()->ba_iter().Skip(-me()->pos());

	ArrayData data(me()->size());
	me()->Read(data, 0, data.size());

	return data;
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

BigInt size() const
{
	return me()->is_iter().getRawKey(1);
}

BigInt pos() const
{
	return me()->ba_iter().pos() - me()->is_iter().prefixes()[1];
}

BigInt getKey() const
{
	return me()->is_iter().getKey(0);
}

IdxsetAccumulator getKeys() const
{
	IdxsetAccumulator keys = me()->is_iter().getRawKeys();

	keys[0] += me()->is_iter().prefix();

	return keys;
}

bool IsNotEnd() const
{
	return me()->is_iter().IsNotEnd();
}

bool IsEnd() const
{
	return me()->is_iter().IsEnd();
}

bool IsBegin() const
{
	return me()->is_iter().IsBegin();
}

bool IsEmpty() const
{
	return me()->is_iter().IsEmpty();
}

bool Next() {
	return me()->NextKey();
}

bool NextKey()
{
	me()->ba_iter().Skip(size() - pos());
	return me()->is_iter().Next();
}

void setValue(BigInt value)
{
	ArrayData data(sizeof(value), &value);
	me()->Update(data);
}

bool operator++() {
	return me()->NextKey();
}

bool operator--() {
	return me()->PrevKey();
}

bool operator++(int) {
	return me()->NextKey();
}

bool operator--(int) {
	return me()->PrevKey();
}

BigInt key() const {
	return me()->is_iter().key();
}

MyType& operator*() {
	return *me();
}

void setValue(StringRef value)
{
	ArrayData data(value.size(), T2T<UByte*>(value.c_str()));
	me()->Update(data);
}

void setValue(const ArrayData& value)
{
	me()->Update(value);
}

operator ArrayData()
{
	ArrayData data(me()->size());
	BigInt len = me()->Read(data);
	me()->Skip(-len);
	return data;
}

operator String ()
{
	ArrayData data(me()->size());
	BigInt len = me()->Read(data);
	me()->Skip(-len);
	return String(T2T<char*>(data.data()), data.size());
}

MEMORIA_ITERATOR_PART_END

}


#endif
