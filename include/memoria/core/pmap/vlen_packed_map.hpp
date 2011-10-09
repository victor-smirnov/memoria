
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_PMAP_PACKED_MAP_H
#define _MEMORIA_CORE_TOOLS_PMAP_PACKED_MAP_H

#include <memoria/core/tools/buffer.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/pmap/vlen_packed_map_intrnl.hpp>
#include <memoria/core/pmap/vlen_integer.hpp>

#include <memoria/core/tools/reflection.hpp>

#include <iostream>
#include <typeinfo>
#include <malloc.h>

namespace memoria {
namespace vlen {


template<template<typename > class Accumulator = Tracker,
		template<typename > class CmpLT = CompareLT,
		template<typename > class CmpLE = CompareLE,
		template<typename > class CmpEQ = CompareEQ>
class VLenPackedMap {
public:
	typedef VLenInteger KeyType;
	typedef VLenInteger IndexKeyType;
	typedef VLenInteger ValueType;

public:
	typedef KeyType Key;
	typedef ValueType Value;
	typedef IndexKeyType IndexKey;

	typedef VLenPackedMap<Accumulator, CmpLT, CmpLE, CmpEQ> Me;

	static const Int kHashCode = 345435322;

private:




	Int size_;
	Int max_size_;
	Int indexes_;
	Int children_;
	Int index_size_;
	Int value_size_;
	Int data_offset_;
	Int data_block_size_;

	Int data_[];

public:

	//Int block_size, Int children_, Int indexes, const Int* key_sizes, Int value_size
	VLenPackedMap() {
//		init(block_size, children_, indexes, key_sizes, value_size);
	}

	void init(Int block_size, Int children, Int indexes, const Int* key_sizes, Int value_size)
	{
		size_ 		= 0;
		children_ 	= children;
		value_size_ = value_size;
		indexes_	= indexes;


		data_[0]	= 0;
		Int sum 	= data_[1] = key_sizes[0];
		for (Int c = 1; c < indexes; c++)
		{
			Int tmp = key_sizes[c] + sum;
			sum = data_[c + 1] = tmp;
		}

		Int item_size = sum * indexes_ + value_size_;
		data_block_size_ = item_size;

		block_size = (block_size - sizeof(Me)) / sizeof(Int);

		Int first = 1;
		Int last = block_size / item_size;
		while (first < last - 1)
		{
			Int middle = (first + last) / 2;

			Int size = get_block_size(middle);
			if (size < block_size)
			{
				first = middle;
			}
			else if (size > block_size)
			{
				last = middle;
			}
			else {
				break;
			}
		}

		if (get_block_size(last) <= block_size)
		{
			max_size_ = last;
		}
		else if (get_block_size((first + last) / 2) <= block_size)
		{
			max_size_ = (first + last) / 2;
		}
		else {
			max_size_ = first;
		}

		index_size_ = get_index_size(max_size_);

		data_offset_ = index_size_ * index_keys_size() * indexes_;

//		cout<<size_<<endl;
//		cout<<max_size_<<endl;
//		cout<<indexes_<<endl;
//		cout<<children_<<endl;
//		cout<<index_size_<<endl;
//		cout<<value_size_<<endl;
//		cout<<data_offset_<<endl;
//		cout<<data_block_size_<<endl;
//
//		cout<<"==="<<endl;
//		for (Int c = 0; c < indexes + 1; c++) {
//			cout<<data_[c]<<endl;
//		}
	}

	const Int &size() const
	{
		return size_;
	}

	Int &size()
	{
		return size_;
	}

	Int key_offset(Int i) const
	{
		return data_[i];
	}

	Int keys_size() const
	{
		return data_[indexes_];
	}

	Int index_size() const
	{
		return index_size_;
	}

	Int key_size(Int i) const
	{
		return key_offset(i + 1) - key_offset(i);
	}

	Int index_key_offset(Int i) const
	{
		return key_offset(i);
	}

	Int index_key_size(Int i) const
	{
		return key_size(i);
	}

	Int index_keys_size() const
	{
		return keys_size();
	}

	Int value_size() const
	{
		return value_size_;
	}

	UInt max_size() const
	{
		return max_size_;
	}

	Int indexes() const
	{
		return indexes_;
	}

	const IndexKey index(Int n) const
	{
		return index(0, n);
	}

	IndexKey index(Int n)
	{
		return index(0, n);
	}

	const Int* ptr() const
	{
		return data_;
	}

	Int* ptr()
	{
		return data_;
	}

//	const IndexKey index(Int i, Int n)
//	{
//		Int idx = index_keys_size() * n + index_key_offset(i);
//		return IndexKey(index_key_size(i), const_cast<Int*>(ptr() + idx));
//	}

	IndexKey index(Int i, Int n)
	{
		Int idx = indexes_ + 1 + index_keys_size() * n + index_key_offset(i);
		return IndexKey(index_key_size(i), ptr() + idx);
	}

//	const KeyType key(Int n) const {
//		return key(0, n);
//	}

	KeyType key(Int n) {
		return key(0, n);
	}

//	const KeyType key(Int i, Int n) const
//	{
//		Int idx = data_offset_ + n * data_block_size_ + key_offset(i);
//		return KeyType(key_size(i), const_cast<Int*>(ptr() + idx));
//	}

	KeyType key(Int i, Int n)
	{
		Int idx = data_offset_ + n * data_block_size_ + key_offset(i);
		return KeyType(key_size(i), ptr() + idx);
	}

//	TODO: Return BigInteger*
//	const KeyType* keys(Int n) const {
//		Int idx = data_offset() + n * (key_size(i) * Indexes + value_size());
//		return (KeyType*) (ptr() + idx);
//	}

//	KeyType* keys(Int n) {
//		Int idx = data_offset() + n * (key_size(i) * Indexes + value_size());
//		return (KeyType*) (base::ptr() + idx);
//	}

//	const ValueType data(Int n) const
//	{
//		Int idx = data_offset_ + n * data_block_size_ + key_offset(indexes_);
//		return ValueType(value_size(), ptr() + idx);
//	}

	ValueType data(Int n)
	{
		Int idx = data_offset_ + n * data_block_size_ + key_offset(indexes_);
		return ValueType(value_size(), ptr() + idx);
	}

//	Int Add(const KeyType &k, const ValueType &value)
//	{
//		set(size(), k, value);
//		size()++;
//		return size();
//	}

	Int Add(const KeyType& k)
	{
		key(size()) = k;
		size()++;
		return size();
	}

//	void set(Int idx, const KeyType& k, const ValueType &value) {
//		set(0, idx, k, value);
//	}
//
//	void set(Int i, Int idx, const KeyType& k, const ValueType &value) {
//		set_key(i, idx, k);
//		set_data(i, idx, value);
//	}

	IndexKey max_key()
	{
		return max_key(0);
	}

	IndexKey max_key(Int i)
	{
		return index(i, 0);
	}

	const KeyType min_key() const
	{
		return min_key(0);
	}

	const KeyType min_key(Int i) const
	{
		return key(i, 0);
	}

	void Reindex()
	{
		for (Int c = 0; c < indexes(); c++)
		{
			UByte acc_buf[index_key_size(c)];
			IndexKeyType key(index_key_size(c), acc_buf);
			Accumulator<IndexKeyType> acc(key);

			UByte acc1_buf[index_key_size(c)];
			IndexKeyType key1(index_key_size(c), acc1_buf);
			Accumulator<IndexKeyType> acc1(key1);

			acc.Reset();
			acc1.Reset();

			Reindex(c, acc, acc1);
		}
	}

	void Reindex(Int index_num, Accumulator<IndexKeyType> &acc, Accumulator<IndexKeyType> &acc1 )
	{
		Int _size = size();
		if (_size > 0)
		{
			Int isize = get_index_size(_size);
			Int size0 = get_parent_nodes_for(_size);
//			IndexKeyType
//			Accumulator<IndexKeyType> acc;
			Int idx;
			for (idx = 0; idx < (_size - _size % children_); idx += children_)
			{

				for (Int i = idx; i < idx + children_; i++)
				{
					acc(key(index_num, i));
				}

				index(index_num, isize - size0 + (idx / children_)) = acc.get();
				acc.Reset();
			}

			if ((_size % children_) != 0)
			{
				for (Int i = idx; i < _size; i++) {
					acc(key(index_num, i));
				}

				index(index_num, isize - size0 + (idx / children_)) = acc.get();
			}

			Int i_base = isize - size0;
			_size = size0;

			while (size0 > 1)
			{
				size0 = get_parent_nodes_for(_size);
//				Accumulator<IndexKeyType> acc1;
				acc1.Reset();

				for (idx = 0; idx < (_size - _size % children_); idx += children_)
				{
					for (Int i = idx + i_base; i < idx + children_ + i_base; i++)
					{
						acc1(index(index_num, i));
					}

					index(index_num, i_base - size0 + (idx / children_)) = acc1.get();
					acc1.Reset();
				}

				if ((_size % children_) != 0)
				{
					for (Int i = idx + i_base; i < _size + i_base; i++)
					{
						acc1(index(index_num, i));
					}

					index(index_num, i_base - size0 + (idx / children_)) = acc1.get();
				}

				i_base -= size0;
				_size = size0;
			}
		}
	}

	template<typename IndexComparator, typename DataComparator>
	Int Find(Int index_num, const KeyType& k, IndexComparator &index_comparator, DataComparator &data_comparator)
	{
		if (data_comparator.TestMax(k, max_key(index_num))) {
			return -1;
		}

		Int levels = get_levels_for_size(size());
		Int base = 0, start = 0;

		for (Int level = 0; level < levels; level++)
		{
			Int size = get_elements_on_level(level);

			for (Int idx = start; idx < size; idx++)
			{
				const KeyType key0 = index(index_num, base + idx);
				if (index_comparator(k, key0))
				{
					start = (idx * children_);
					index_comparator.Sub(key0);
					break;
				}
			}

			base += size;
		}

		if (data_comparator.isShouldReset())
		{
			data_comparator.Reset(index_comparator.get());
		}

		Int _size = size();
		Int stop = (start + children_) > _size ? _size : start + children_;
		for (Int idx = start; idx < stop; idx++)
		{
			//KeyType key0 = key(index_num, idx);
			if (data_comparator(k, key(index_num, idx)))
			{
				return idx;
			}
		}

		return -1;
	}

	Int FindLT(const KeyType& k) const {
		return FindLT(0, k);
	}

	Int FindLT(Int i, const KeyType& k) const {
		CmpLT<IndexKeyType> lt0, lt1;
		return Find(i, k, lt0, lt1);
	}

	Int FindLTS(const KeyType& k, KeyType &sum) const {
		return FindLT(0, k, sum);
	}

	Int FindLTS(Int i, const KeyType& k, KeyType &sum) const {
		CmpLT<IndexKeyType> lt0, lt1;
		Int idx = Find(i, k, lt0, lt1);
		sum += lt1.get();
		sum -= (size() > 0 ? key(i, idx) : 0);
		return idx;
	}

	Int FindLE(const KeyType& k) const {
		return FindLE(0, k);
	}

	Int FindLE(Int i, const KeyType& k) const {
		CmpLE<IndexKeyType> le0, le1;
		return Find(i, k, le0, le1);
	}

	Int FindLES(const KeyType& k, KeyType &sum) const {
		return FindLES(0, k, sum);
	}

	Int FindLES(Int i, const KeyType& k, KeyType &sum) const {
		CmpLE<IndexKeyType> le0, le1;
		Int idx = Find(i, k, le0, le1);
		sum += le1.get() - (size() > 0 ? key(i, idx) : 0);
		return idx;
	}

	Int FindEQ(const KeyType& k){
		return FindEQ(0, k);
	}

	Int FindEQ(Int i, const KeyType& k)
	{
		CmpLE<IndexKeyType> le;
		CmpEQ<IndexKeyType> eq;
		return Find(i, k, le, eq);
	}

	KeyType Prefix(Int i, Int idx, const KeyType& sum) const
	{
		return sum - key(i, idx);
	}

	void MoveData(Int dest, Int src, Int count, const KeyType& k, const ValueType &value)
	{
		Int c;
		if (src < dest)
		{
			--src;
			--dest;
			for (c = count; c > 0; c--)
			{
				key(dest + c) = key(src + c);
				key(src + c) = k;
				if (value_size() > 0)
				{
					data(dest + c) = data(src + c);
					data(src + c) = value;
				}
			}
		}
		else {
			for (c = 0; c < count; c++)
			{
				key(dest + c) = key(src + c);
				key(src + c) = k;
				if (value_size() > 0) {
					data(dest + c) = data(src + c);
					data(src + c) = value;
				}
			}
		}
	}

	void MoveData(Int dest, Int src, Int count)
	{
		Int c;
		if (src < dest)
		{
			--src;
			--dest;
			for (c = count; c > 0; c--)
			{
				for (Int j = 0; j < indexes(); j++)
				{
					key(j, dest + c) = key(j, src + c);
				}

				if (value_size() > 0)
				{
					data(dest + c) = data(src + c);
				}
			}
		}
		else {
			for (c = 0; c < count; c++)
			{
				for (Int j = 0; j < indexes(); j++)
				{
					key(j, dest + c) = key(j, src + c);
				}

				if (value_size() > 0)
				{
					data(dest + c) = data(src + c);
				}
			}
		}
	}

	void CopyData(Int from, Int count, Me &other, Int to) const
	{
		for (Int c = from; c < from + count; c++) {
			for (Int j = 0; j < indexes(); j++) {
				other.key(j, to + c - from) = key(j, c);
			}

			if (value_size() > 0) {
				other.data(to + c - from) = data(c);
			}
		}
	}

	void MoveData(Int dest, Int src, Int count, const KeyType& k)
	{
		MoveData(dest, src, ValueType());
	}

	//TODO: needs testing
	void Remove(Int idx, Int count)
	{
		const Int size0 = size();
		MoveData(idx, idx + count, size0 - idx - count);
		size() -= count;

		for (Int c = size(); c < size0; c++)
		{
			key(c) = 0;
		}
	}

	Int get_index_levels_count() const
	{
		return get_levels_for_size(size());
	}

	Int get_elements_on_level(Int level) const
	{
		Int _size = size();
		Int levels = get_levels_for_size(_size);

		for (Int nlevels = levels - 1; nlevels >= level; nlevels--)
		{
			_size = ((_size % children_) == 0) ? (_size / children_) : (_size / children_) + 1;
		}

		return _size;
	}

	void MoveTo(Me &target, Int from) const
	{
		Int count = size() - from;

		if (target.size() > 0) {
			target.MoveData(count, 0, target.size());
		}

		for (Int i = from; i < size(); i++)
		{
			target.set_key(i - from, key(i));
			if (value_size() > 0)
			{
				target.set_data(i - from, data(i));
			}
		}

		Reindex();
		target.Reindex();
	}

protected:

	Int get_index_size(Int csize) const
	{
		if (csize == 1)
		{
			return 1;
		}
		else {
			Int sum = 0;
			for (Int nlevels = 0; csize > 1; nlevels++)
			{
				csize = ((csize % children_) == 0) ? (csize / children_) : (csize / children_) + 1;
				sum += csize;
			}
			return sum;
		}
	}

	Int get_levels_for_size(Int csize) const
	{
		Int nlevels;
		for (nlevels = 0; csize > 1; nlevels++)
		{
			csize = ((csize % children_) == 0) ? (csize / children_) : (csize	/ children_) + 1;
		}
		return nlevels;
	}

	Int get_parent_nodes_for(Int n) const
	{
		return (n % children_) == 0 ? n / children_ : ((n / children_) + 1);
	}

	Int get_block_size(Int item_count) const
	{
		Int key_size = index_keys_size();
		Int item_size = data_block_size_;
		return get_index_size(item_count) * key_size + item_count * item_size;
	}

};


} //tools
} //memoria


#endif
