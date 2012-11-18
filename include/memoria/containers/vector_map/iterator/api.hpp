
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

typedef Ctr<VectorMapCtrTypes<Types> >                      ContainerType;

typedef typename ContainerType::Idxset::Iterator            IdxsetIterator;
typedef typename ContainerType::ByteArray::Iterator         ByteArrayIterator;

typedef typename ContainerType::Idxset::Accumulator         IdxsetAccumulator;

typedef typename Types::Profile                             Profile;
typedef typename Types::Allocator                           Allocator;
typedef typename Types::Allocator::CtrShared                CtrShared;
typedef typename Types::Allocator::PageG                    PageG;
typedef typename Allocator::Page                            Page;
typedef typename Page::ID                                   ID;

typedef typename ContainerType::Key                         Key;




void insert(const ArrayData& data)
{
    me()->ba_iter().insert(data);

    IdxsetAccumulator keys;

    keys.key(1) = data.size();

    me()->is_iter().updateUp(keys);
}

void update(const ArrayData& data)
{
    BigInt sz = me()->size();

    if (sz > 0)
    {
        BigInt difference = sz - data.size();

        if (difference < 0)
        {
            me()->ba_iter().update(data);
            me()->ba_iter().remove(difference);
        }
        else if (difference > 0)
        {
            me()->ba_iter().update(data, 0, sz);
            me()->ba_iter().insert(data, sz, difference);
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

    me()->ba_iter().skip(-data.size());
}


ArrayData read()
{
    me()->ba_iter().skip(-me()->pos());

    ArrayData data(me()->size());
    me()->read(data, 0, data.size());

    return data;
}


BigInt read(ArrayData& data)
{
    return read(data, 0, data.size());
}


BigInt read(ArrayData& data, BigInt start, BigInt length)
{
    BigInt current_pos  = me()->pos();
    BigInt current_size = me()->size();

    BigInt len = ((length + current_pos) <= current_size) ? length : (current_size - current_pos);

    me()->ba_iter().read(data, start, len);

    return len;
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
    BigInt data_size = me()->size();
    me()->model().set().removeEntry(me()->is_iter());
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

void setValue(BigInt value)
{
    ArrayData data(sizeof(value), &value);
    me()->update(data);
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

void setValue(StringRef value)
{
    ArrayData data(value.size(), T2T<UByte*>(value.c_str()));
    me()->update(data);
}

void setValue(const ArrayData& value)
{
    me()->update(value);
}

operator ArrayData()
{
    ArrayData data(me()->size());
    BigInt len = me()->read(data);
    me()->skip(-len);
    return data;
}

operator String ()
{
    ArrayData data(me()->size());
    BigInt len = me()->read(data);
    me()->skip(-len);
    return String(T2T<char*>(data.data()), data.size());
}

MEMORIA_ITERATOR_PART_END

}


#endif
