
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PMAP_PACKED_MAP2_HPP_
#define MEMORIA_CORE_PMAP_PACKED_MAP2_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/bitmap.hpp>



namespace memoria {

template <typename Types>
class PackedMap2 {

	typedef PackedMap2<Types>				MyType;

public:

	typedef typename Types::Key				Key;
	typedef typename Types::IndexKey		IndexKey;
	typedef typename Types::Value			Value;
	typedef typename Types::Accumulator		Accumulator;

	static const Int Blocks					= Types::Blocks;
	static const Int BranchingFactor		= Types::BranchingFactor;

private:
	Int 	size_;
	Int 	max_size_;
	Int		index_size_;
	Byte 	memory_block_[];

public:
	PackedMap2() {}

	void InitByBlock(Int block_size)
	{
		size_ = 0;

		max_size_	= GetMaxSize(block_size);
		index_size_ = GetIndexSize(max_size_);
	}


	void InitSizes(Int max)
	{
		size_		= 0;
		max_size_	= max;
		index_size_ = GetIndexSize(max_size_);
	}

	Int GetObjectSize() const
	{
		return sizeof(MyType) + GetBlockSize();
	}

	Int GetObjectDataSize() const
	{
		return sizeof(size_) + sizeof(max_size_) + sizeof(index_size_) + GetBlockSize();
	}

	Int GetBlockSize() const
	{
		return (index_size_ * sizeof(IndexKey) + max_size_ * sizeof(Key)) * Blocks + max_size_ * GetValueSize();
	}

	Int& size() {
		return size_;
	}

	const Int& size() const
	{
		return size_;
	}

	Int index_size() const
	{
		return index_size_;
	}

	Int max_size() const
	{
		return max_size_;
	}

	Byte* memory_block() {
		return memory_block_;
	}

	const Byte* memory_block() const {
		return memory_block_;
	}

	Int GetIndexKeyBlockOffset(Int block_num) const
	{
		return GetIndexKeyBlockOffset(index_size_, block_num);
	}

	Int GetIndexKeyBlockOffset(Int index_size, Int block_num) const
	{
		return sizeof(IndexKey) * index_size * block_num;
	}

	Int GetKeyBlockOffset(Int block_num) const
	{
		return GetKeyBlockOffset(index_size_, max_size_, block_num);
	}

	Int GetKeyBlockOffset(Int index_size, Int keys_size, Int block_num) const
	{
		return GetIndexKeyBlockOffset(index_size, Blocks) + sizeof(Key) * keys_size * block_num;
	}

	Int GetValueBlockOffset() const
	{
		return GetValueBlockOffset(index_size_, max_size_);
	}

	Int GetValueBlockOffset(Int index_size, Int keys_size) const
	{
		return GetKeyBlockOffset(index_size, keys_size, Blocks);
	}

	template <typename T>
	T& block_item(Int block_offset, Int item_idx, Int item_size = sizeof(T))
	{
		return *T2T<T*>(memory_block_ + block_offset + item_idx * item_size);
	}

	template <typename T>
	const T& block_item(Int block_offset, Int item_idx, Int item_size = sizeof(T)) const
	{
		return *T2T<const T*>(memory_block_ + block_offset + item_idx * item_size);
	}

	IndexKey& indexb(Int block_offset, Int key_num)
	{
		return block_item<IndexKey>(block_offset, key_num);
	}

	const IndexKey& indexb(Int block_offset, Int key_num) const
	{
		return block_item<IndexKey>(block_offset, key_num);
	}

	IndexKey& index(Int block_num, Int key_num)
	{
		return block_item<IndexKey>(GetIndexKeyBlockOffset(block_num), key_num);
	}

	const IndexKey& index(Int block_num, Int key_num) const
	{
		return block_item<IndexKey>(GetIndexKeyBlockOffset(block_num), key_num);
	}


	Key& keyb(Int block_offset, Int key_num)
	{
		return block_item<Key>(block_offset, key_num);
	}

	const Key& keyb(Int block_offset, Int key_num) const
	{
		return block_item<Key>(block_offset, key_num);
	}

	Key& key(Int block_num, Int key_num)
	{
		return block_item<Key>(GetKeyBlockOffset(block_num), key_num);
	}

	const Key& key(Int block_num, Int key_num) const
	{
		return block_item<Key>(GetKeyBlockOffset(block_num), key_num);
	}

	Value& value(Int value_num)
	{
		return block_item<Value>(GetValueBlockOffset(), value_num, GetValueSize());
	}

	const Value& value(Int value_num) const
	{
		return block_item<Value>(GetValueBlockOffset(), value_num, GetValueSize());
	}

	static Int GetValueSize()
	{
		return sizeof(Value);
	}

	void Enlarge(Byte* target_memory_block, Int number)
	{
		Int new_keys_size	= max_size_ + number;

		Int new_index_size 	= GetIndexSize(new_keys_size);

		Int value_size = GetValueSize();

		if (value_size > 0)
		{
			Int offset		= GetValueBlockOffset();
			Int new_offset	= GetValueBlockOffset(new_index_size, new_keys_size);

			CopyData(target_memory_block, offset, new_offset, value_size);
		}

		for (Int c = Blocks - 1; c >= 0; c--)
		{
			Int offset		= GetKeyBlockOffset(c);
			Int new_offset	= GetKeyBlockOffset(new_index_size, new_keys_size, c);

			CopyData(target_memory_block, offset, new_offset, sizeof(Key));
		}

		for (Int c = Blocks - 1; c >= 0; c--)
		{
			Int offset		= GetIndexKeyBlockOffset(c);
			Int new_offset	= GetIndexKeyBlockOffset(new_index_size, c);

			CopyIndex(target_memory_block, offset, new_offset, sizeof(IndexKey));
		}
	}

	void EnlargeBlock(Int block_size)
	{
		Int max_size = GetMaxSize(block_size);

		Enlarge(memory_block_, max_size - max_size_);

		max_size_ 	= max_size;
		index_size_	= GetIndexSize(max_size_);
	}


	void Shrink(Byte* target_memory_block, Int number)
	{
		Int new_keys_size	= max_size_ - number;

		Int new_index_size 	= GetIndexSize(new_keys_size);

		Int value_size = GetValueSize();

		for (Int c = 0; c < Blocks; c++)
		{
			Int offset		= GetIndexKeyBlockOffset(c);
			Int new_offset	= GetIndexKeyBlockOffset(new_index_size, c);

			CopyIndex(target_memory_block, offset, new_offset, sizeof(IndexKey));
		}

		for (Int c = 0; c < Blocks; c++)
		{
			Int offset		= GetKeyBlockOffset(c);
			Int new_offset	= GetKeyBlockOffset(new_index_size, new_keys_size, c);

			CopyData(target_memory_block, offset, new_offset, sizeof(Key));
		}

		if (value_size > 0)
		{
			Int offset		= GetValueBlockOffset();
			Int new_offset	= GetValueBlockOffset(new_index_size, new_keys_size);

			CopyData(target_memory_block, offset, new_offset, value_size);
		}
	}


	void ShrinkBlock(Int block_size)
	{
		Int max_size = GetMaxSize(block_size);

		Shrink(memory_block_, max_size_ - max_size);

		max_size_ 	= max_size;
		index_size_	= GetIndexSize(max_size_);
	}


	void InsertSpace(Int room_start, Int room_length)
	{
		Int value_size = GetValueSize();

		if (value_size > 0)
		{
			Int offset = GetValueBlockOffset();

			CopyData(offset, room_start, room_length, value_size);
		}

		for (Int c = Blocks - 1; c >= 0; c--)
		{
			Int offset = GetKeyBlockOffset(c);

			CopyData(offset, room_start, room_length, sizeof(Key));
		}

		size_ += room_length;
	}

	void RemoveSpace(Int room_start, Int room_length)
	{
		Int value_size = GetValueSize();

		for (Int c = 0; c < Blocks; c++)
		{
			Int offset = GetKeyBlockOffset(c);

			CopyData(offset, room_start, -room_length, sizeof(Key));
		}

		if (value_size > 0)
		{
			Int offset = GetValueBlockOffset();

			CopyData(offset, room_start, -room_length, value_size);
		}

		size_ -= room_length;
	}

	void Reindex(Int block_num)
	{
		Reindex(block_num, 0, size());
	}

	void Reindex(Int block_num, Int start, Int end) {}

	void Insert(const Accumulator& keys, const Value& val, Int at)
	{
		if (at < size_ - 1)
		{
			InsertSpace(at, 1);
		}

		for (Int c = 0; c < Blocks; c++)
		{
			key(c, at) = keys[c];
		}

		value(at) = val;
	}

private:

	void CopyData(Byte* target_memory_block, Int offset, Int new_offset, Int item_size)
	{
		CopyBuffer(
				memory_block_ 		+ offset,
				target_memory_block + new_offset,
				max_size_ * item_size
		);
	}

	void CopyIndex(Byte* target_memory_block, Int offset, Int new_offset, Int item_size)
	{
		CopyBuffer(
				memory_block_ 		+ offset,
				target_memory_block + new_offset,
				index_size_ * item_size
		);
	}


	void CopyData(Int offset, Int room_start, Int room_length, Int item_size)
	{
		Byte* src = memory_block_ + offset + room_start * item_size;
		Byte* dst = src + room_length * item_size;

		CopyBuffer(src, dst, (size_ - room_start) * item_size);
	}

	Int GetBlockSize(Int item_count)
	{
		Int key_size  = sizeof(IndexKey) * Blocks;
		Int item_size = sizeof(Key)*Blocks + GetValueSize();
		return GetIndexSize(item_count) * key_size + item_count * item_size;
	}

	Int GetIndexSize(Int csize)
	{
		if (csize == 1)
		{
			return 1;
		}
		else {
			Int sum = 0;
			for (Int nlevels=0; csize > 1; nlevels++)
			{
				csize = ((csize % BranchingFactor) == 0) ? (csize / BranchingFactor) : (csize / BranchingFactor) + 1;
				sum += csize;
			}
			return sum;
		}
	}

	Int GetMaxSize(Int block_size)
	{
		Int item_size 	= sizeof(Key) * Blocks + GetValueSize();

		Int first 		= 1;
		Int last  		= block_size / item_size;



		while (first < last - 1)
		{
			Int middle = (first + last) / 2;

			Int size = GetBlockSize(middle);
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

		Int max_size;

		if (GetBlockSize(last) <= block_size)
		{
			max_size = last;
		}
		else if (GetBlockSize((first + last) / 2) <= block_size)
		{
			max_size = (first + last) / 2;
		}
		else {
			max_size = first;
		}

		return max_size;
	}
};

}


#endif
