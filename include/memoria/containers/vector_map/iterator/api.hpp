
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_VECTOR_MAP_ITERATOR_API_HPP
#define _MEMORIA_MODELS_VECTOR_MAP_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/vector_map/names.hpp>
#include <memoria/containers/vector_map/tools.hpp>
#include <memoria/core/container/iterator.hpp>

#include <vector>
#include <iostream>

namespace memoria    {

using namespace std;
using namespace memoria::vector_map;

MEMORIA_ITERATOR_PART_BEGIN(memoria::vector_map::ItrApiName)

typedef Ctr<VectorMapCtrTypes<Types> >                      ContainerType;

typedef typename ContainerType::IdxSet::Iterator            IdxsetIterator;
typedef typename ContainerType::ByteArray::Iterator         ByteArrayIterator;

typedef typename ContainerType::IdxSet::Accumulator         IdxsetAccumulator;

typedef typename Types::Profile                             Profile;
typedef typename Types::Allocator                           Allocator;
typedef typename Types::Allocator::CtrShared                CtrShared;
typedef typename Types::Allocator::PageG                    PageG;
typedef typename Allocator::Page                            Page;
typedef typename Page::ID                                   ID;

typedef typename ContainerType::Key                         Key;

typedef typename ContainerType::ByteArray::ElementType      ElementType;


void insert(IData<ElementType>& data)
{
    me()->ba_iter().insert(data);

    IdxsetAccumulator keys;

    keys.key(1) = data.getRemainder();

    me()->is_iter().updateUp(keys);
}

void update(IData<ElementType>& data)
{
    BigInt sz = me()->size();

    if (sz > 0)
    {
        BigInt difference = data.getRemainder() - sz;

        if (difference < 0)
        {
            me()->ba_iter().update(data);
            me()->ba_iter().remove(-difference);
        }
        else if (difference > 0)
        {
            DataProxy<ElementType> proxy(data, sz);
            me()->ba_iter().update(proxy);
            me()->ba_iter().insert(data);
        }
        else {
            me()->ba_iter().update(data);
        }

        if (difference != 0)
        {
            IdxsetAccumulator keys;
            keys.key(1) = difference;
            me()->is_iter().updateUp(keys);
        }
    }
    else {
        me()->insert(data);
    }

    me()->ba_iter().skip(-data.getSize());
}


vector<ElementType> read()
{
    me()->ba_iter().skip(-me()->pos());

    return me()->ba_iter().subVector(me()->size());
}




BigInt read(IData<ElementType>& data)
{
    BigInt current_pos  = me()->pos();
    BigInt current_size = me()->size();

    SizeT length = data.getRemainder();

    BigInt len = ((length + current_pos) <= current_size) ? length : (current_size - current_pos);

    me()->ba_iter().read(data);

    return len;
}

BigInt seek(BigInt position)
{
    BigInt current_pos  = me()->pos();
    BigInt current_size = me()->size();

    if (position >= current_size)
    {
        position = current_pos;
    }
    else if (position <= 0)
    {
        position = 0;
    }

    return skip(position - current_pos);
}


BigInt skip(BigInt length)
{
    BigInt current  = me()->pos();

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

    return me()->ba_iter().skip(length);
}

void remove()
{
    me()->erase();

    me()->model().set().removeEntry(me()->is_iter());
}

void erase()
{
    BigInt current = me()->pos();

    if (pos > 0)
    {
        me()->skip(-current);
    }

    BigInt data_size = me()->size();

    me()->ba_iter().remove(data_size);
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

bool isNotEnd() const
{
    return me()->is_iter().isNotEnd();
}

bool isEnd() const
{
    return me()->is_iter().isEnd();
}

bool isBegin() const
{
    return me()->is_iter().isBegin();
}

bool isEmpty() const
{
    return me()->is_iter().isEmpty();
}

bool next() {
    return me()->nextKey();
}

bool nextKey()
{
    me()->ba_iter().skip(size() - pos());
    return me()->is_iter().next();
}

bool operator++() {
    return me()->nextKey();
}

bool operator--() {
    return me()->prevKey();
}

bool operator++(int) {
    return me()->nextKey();
}

bool operator--(int) {
    return me()->prevKey();
}

BigInt key() const {
    return me()->is_iter().key();
}

MyType& operator*() {
    return *me();
}

void setValue(IData<ElementType>& value)
{
    me()->update(value);
}

void setValue(const vector<ElementType>& value)
{
    MemBuffer<const ElementType> buffer(&value[0], value.size());

    me()->update(buffer);
}

operator vector<ElementType>()
{
    vector<ElementType> vec(me()->size());

    MemBuffer<ElementType> data(&vec[0], vec.size());

    BigInt len = me()->read(data);
    me()->skip(-len);
    return data;
}

VectorMapDataWrapper<IDataAdapter<ByteArrayIterator>> asData() const
{
    return VectorMapDataWrapper<IDataAdapter<ByteArrayIterator>>(me()->ba_iter().asData(me()->size()));
}

MEMORIA_ITERATOR_PART_END

}


#endif
