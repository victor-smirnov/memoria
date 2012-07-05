
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PMAP_PACKED_TREE_HPP_
#define MEMORIA_CORE_PMAP_PACKED_TREE_HPP_

#include <memoria/core/container/page_traits.hpp>
#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/traits.hpp>
#include <memoria/core/tools/buffer.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/tools/reflection.hpp>


namespace memoria {


namespace intrnl0 {

template <typename T>
struct ValueHelper {
	static void setup(IPageDataEventHandler* handler, const T& value)
	{
		handler->Value("VALUE", &value);
	}
};

template <typename T, size_t Size>
struct ValueHelper<AbstractPageID<T, Size> > {
	typedef AbstractPageID<T, Size> Type;

	static void setup(IPageDataEventHandler* handler, const Type& value)
	{
		IDValue id(&value);
		handler->Value("VALUE", &id);
	}
};

template <>
struct ValueHelper<EmptyValue> {
	typedef EmptyValue Type;

	static void setup(IPageDataEventHandler* handler, const Type& value)
	{
		BigInt val = 0;
		handler->Value("VALUE", &val);
	}
};


}



template <typename Types>
class PackedTree {

	typedef PackedTree<Types>				MyType;

public:

	typedef typename Types::Key				Key;
	typedef typename Types::IndexKey		IndexKey;
	typedef typename Types::Value			Value;
	typedef typename Types::Accumulator		Accumulator;

	static const Int Blocks					= Types::Blocks;
	static const Int BranchingFactor		= Types::BranchingFactor;

	template <typename T> friend class PackedTree;

private:

	static const Int LEVELS_MAX				= 32;

	Int 	size_;
	Int 	max_size_;
	Int		index_size_;
	Byte 	memory_block_[];

public:
	PackedTree() {}


	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		handler->StartGroup("PACKED_TREE");

		handler->Value("SIZE", 			&size_);
		handler->Value("MAX_SIZE", 		&max_size_);
		handler->Value("INDEX_SIZE", 	&index_size_);

		handler->StartGroup("INDEXES", index_size_);

		for (Int idx = 0; idx < index_size_; idx++)
		{
			IndexKey indexes[Blocks];
			for (Int block = 0; block < Blocks; block++)
			{
				indexes[block] = index(block, idx);
			}

			handler->Value("INDEX", indexes, Blocks);
		}

		handler->EndGroup();

		handler->StartGroup("DATA", size_);

		for (Int idx = 0; idx < size_; idx++)
		{
			handler->StartLine("ENTRY");

			Key keys[Blocks];
			for (Int block = 0; block < Blocks; block++)
			{
				keys[block] = key(block, idx);
			}

			handler->Value(Blocks == 1 ? "KEY" : "KEYS", keys, Blocks);

			if (getValueSize() > 0)
			{
				intrnl0::ValueHelper<Value>::setup(handler, value(idx));
			}

			handler->EndLine();
		}

		handler->EndGroup();

		handler->EndGroup();
	}

	void serialize(SerializationData& buf) const
	{
		FieldFactory<Int>::serialize(buf, size());
		FieldFactory<Int>::serialize(buf, max_size_);
		FieldFactory<Int>::serialize(buf, index_size_);

		FieldFactory<IndexKey>::serialize(buf, index(0, 0), Blocks * index_size());

		for (Int c = 0; c < Blocks; c++)
		{
			FieldFactory<Key>::serialize(buf, key(c, 0), size());
		}

		if (getValueSize() > 0)
		{
			FieldFactory<Value>::serialize(buf, value(0), size());
		}
	}

	void deserialize(DeserializationData& buf)
	{
		FieldFactory<Int>::deserialize(buf, size());
		FieldFactory<Int>::deserialize(buf, max_size_);
		FieldFactory<Int>::deserialize(buf, index_size_);

		FieldFactory<IndexKey>::deserialize(buf, index(0, 0), Blocks * index_size());

		for (Int c = 0; c < Blocks; c++)
		{
			FieldFactory<Key>::deserialize(buf, key(c, 0), size());
		}

		if (getValueSize() > 0)
		{
			FieldFactory<Value>::deserialize(buf, value(0), size());
		}
	}




	void InitByBlock(Int block_size)
	{
		size_ = 0;

		max_size_	= getMaxSize(block_size);
		index_size_ = getIndexSize(max_size_);
	}


	void InitSizes(Int max)
	{
		size_		= 0;
		max_size_	= max;
		index_size_ = getIndexSize(max_size_);
	}

	Int getObjectSize() const
	{
		return sizeof(MyType) + getBlockSize();
	}

	Int getObjectDataSize() const
	{
		return sizeof(size_) + sizeof(max_size_) + sizeof(index_size_) + getBlockSize();
	}


	Int getBlockSize() const
	{
		return (index_size_ * sizeof(IndexKey) + max_size_ * sizeof(Key)) * Blocks + max_size_ * getValueSize();
	}

	Int getDataSize() const
	{
		return (index_size_ * sizeof(IndexKey) + size_ * sizeof(Key)) * Blocks + size_ * getValueSize();
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

	static Int max_size_for(Int block_size)
	{
		return getMaxSize(block_size);
	}

	static Int getMemoryBlockSizeFor(Int max)
	{
		Int index_size = getIndexSize(max);
		return (index_size * sizeof(IndexKey) + max * sizeof(Key)) * Blocks + max * getValueSize();
	}

	Byte* memory_block() {
		return memory_block_;
	}

	const Byte* memory_block() const {
		return memory_block_;
	}

	Int getIndexKeyBlockOffset(Int block_num) const
	{
		return getIndexKeyBlockOffset(index_size_, block_num);
	}

	Int getIndexKeyBlockOffset(Int index_size, Int block_num) const
	{
		return sizeof(IndexKey) * index_size * block_num;
	}

	Int getKeyBlockOffset(Int block_num) const
	{
		return getKeyBlockOffset(index_size_, max_size_, block_num);
	}

	Int getKeyBlockOffset(Int index_size, Int keys_size, Int block_num) const
	{
		return getIndexKeyBlockOffset(index_size, Blocks) + sizeof(Key) * keys_size * block_num;
	}

	Int getValueBlockOffset() const
	{
		return getValueBlockOffset(index_size_, max_size_);
	}

	Int getValueBlockOffset(Int index_size, Int keys_size) const
	{
		return getKeyBlockOffset(index_size, keys_size, Blocks);
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
		return block_item<IndexKey>(getIndexKeyBlockOffset(block_num), key_num);
	}

	const IndexKey& index(Int block_num, Int key_num) const
	{
		return block_item<IndexKey>(getIndexKeyBlockOffset(block_num), key_num);
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
		return block_item<Key>(getKeyBlockOffset(block_num), key_num);
	}

	const Key& key(Int block_num, Int key_num) const
	{
		return block_item<Key>(getKeyBlockOffset(block_num), key_num);
	}

	IndexKey& max_key(Int block_num)
	{
		return index(block_num, 0);
	}

	const IndexKey& max_key(Int block_num) const
	{
		return index(block_num, 0);
	}

	const IndexKey& max_keyb(Int block_offset) const
	{
		return indexb(block_offset, 0);
	}

	Value& value(Int value_num)
	{
		return valueb(getValueBlockOffset(), value_num);
	}

	const Value& value(Int value_num) const
	{
		return valueb(getValueBlockOffset(), value_num);
	}

	Value& valueb(Int block_offset, Int value_num)
	{
		return block_item<Value>(block_offset, value_num, getValueSize());
	}

	const Value& valueb(Int block_offset, Int value_num) const
	{
		return block_item<Value>(block_offset, value_num, getValueSize());
	}

	static Int getValueSize()
	{
		return ValueTraits<Value>::Size;
	}

	void CopyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
	{
		for (Int c = 0; c < Blocks; c++)
		{
			Int src_block_offset = this->getKeyBlockOffset(c)  + copy_from * sizeof(Key);
			Int tgt_block_offset = other->getKeyBlockOffset(c) + copy_to * sizeof(Key);

			CopyBuffer(memory_block_ + src_block_offset, other->memory_block_ + tgt_block_offset, count * sizeof(Key));
		}

		if (this->getValueSize() > 0)
		{
			Int src_block_offset = this->getValueBlockOffset()  + copy_from * getValueSize();
			Int tgt_block_offset = other->getValueBlockOffset() + copy_to * getValueSize();

			CopyBuffer(memory_block_ + src_block_offset, other->memory_block_ + tgt_block_offset, count * getValueSize());
		}
	}

	void Clear(Int from, Int to)
	{
		for (Int c = 0; c < Blocks; c++)
		{
			Int block_offset = this->getKeyBlockOffset(c);

			for (Int idx = from; idx < to; idx++)
			{
				keyb(block_offset, idx) = 0;
			}
		}

		if (this->getValueSize() > 0)
		{
			Int block_offset = this->getValueBlockOffset();
			for (Int idx = from; idx < to; idx++)
			{
				valueb(block_offset, idx) = 0;
			}
		}
	}

	void Enlarge(Byte* target_memory_block, Int new_keys_size, Int new_index_size)
	{
		Int value_size = getValueSize();

		if (value_size > 0)
		{
			Int offset		= getValueBlockOffset();
			Int new_offset	= getValueBlockOffset(new_index_size, new_keys_size);

			CopyData(target_memory_block, offset, new_offset, value_size);
		}

		for (Int c = Blocks - 1; c >= 0; c--)
		{
			Int offset		= getKeyBlockOffset(c);
			Int new_offset	= getKeyBlockOffset(new_index_size, new_keys_size, c);

			CopyData(target_memory_block, offset, new_offset, sizeof(Key));
		}

//		for (Int c = Blocks - 1; c >= 0; c--)
//		{
//			Int offset		= getIndexKeyBlockOffset(c);
//			Int new_offset	= getIndexKeyBlockOffset(new_index_size, c);
//
//			CopyIndex(target_memory_block, offset, new_offset, sizeof(IndexKey));
//		}
	}

	void EnlargeBlock(Int block_size)
	{
		Int max_size 	= getMaxSize(block_size);
		Int index_size	= getIndexSize(max_size);

		Enlarge(memory_block_, max_size, index_size);

		max_size_ 	 	= max_size;
		index_size_	 	= index_size;
	}

	void EnlargeTo(MyType* other)
	{
		Enlarge(other->memory_block_, other->max_size_, other->index_size_);
	}

	void Shrink(Byte* target_memory_block, Int new_keys_size, Int new_index_size) const
	{
		Int value_size = getValueSize();

		for (Int c = 0; c < Blocks; c++)
		{
			Int offset		= getKeyBlockOffset(c);
			Int new_offset	= getKeyBlockOffset(new_index_size, new_keys_size, c);

			CopyData(target_memory_block, offset, new_offset, sizeof(Key));
		}

		if (value_size > 0)
		{
			Int offset		= getValueBlockOffset();
			Int new_offset	= getValueBlockOffset(new_index_size, new_keys_size);

			CopyData(target_memory_block, offset, new_offset, value_size);
		}
	}


	void ShrinkBlock(Int block_size)
	{
		Int max_size 	= getMaxSize(block_size);
		Int index_size	= getIndexSize(max_size);

		Shrink(memory_block_, max_size, index_size);

		max_size_ 	 	= max_size;
		index_size_	 	= index_size;
	}

	void ShrinkTo(MyType* other)
	{
		Shrink(other->memory_block_, other->max_size_, other->index_size_);
	}

	template <typename TreeType>
	void TransferTo(TreeType* other) const
	{
		if (sizeof(Key) == sizeof(typename TreeType::Key) && getValueSize() == TreeType::getValueSize())
		{
			Shrink(other->memory_block_, other->max_size_, other->index_size_);
		}
		else {

			for (Int block = 0; block < Blocks; block++)
			{
				Int src_block_offset = this->getKeyBlockOffset(block);
				Int tgt_block_offset = other->getKeyBlockOffset(block);

				for (Int idx = 0; idx < size(); idx++)
				{
					other->keyb(tgt_block_offset, idx) = keyb(src_block_offset, idx);
				}
			}

			if (getValueSize() > 0)
			{
				Int src_block_offset = this->getValueBlockOffset();
				Int tgt_block_offset = other->getValueBlockOffset();

				for (Int idx = 0; idx < size(); idx++)
				{
					other->valueb(tgt_block_offset, idx) = valueb(src_block_offset, idx);
				}
			}
		}
	}

	void insertSpace(Int room_start, Int room_length)
	{
		Int value_size = getValueSize();

		if (value_size > 0)
		{
			Int offset = getValueBlockOffset();

			CopyData(offset, room_start, room_length, value_size);
		}

		for (Int c = Blocks - 1; c >= 0; c--)
		{
			Int offset = getKeyBlockOffset(c);

			CopyData(offset, room_start, room_length, sizeof(Key));
		}

		size_ += room_length;
	}

	void removeSpace(Int room_start, Int room_length)
	{
		Int value_size = getValueSize();

		for (Int c = 0; c < Blocks; c++)
		{
			Int offset = getKeyBlockOffset(c);

			CopyData(offset, room_start + room_length, -room_length, sizeof(Key));
		}

		if (value_size > 0)
		{
			Int offset = getValueBlockOffset();

			CopyData(offset, room_start + room_length, -room_length, value_size);
		}

		size_ -= room_length;
	}

	void Add(Int block_num, Int idx, const Key& value)
	{
		key(block_num, idx) += value;

		Int index_block_offset 	= getIndexKeyBlockOffset(block_num);

		Int index_level_size	= getIndexCellsNumberFor(max_size_);
		Int index_level_start 	= index_size_ - index_level_size;

		while (index_level_start >= 0)
		{
			idx /= BranchingFactor;

			indexb(index_block_offset, idx + index_level_start) += value;

			Int index_parent_size 	= getIndexCellsNumberFor(index_level_size);

			index_level_size 		= index_parent_size;
			index_level_start 		-= index_parent_size;
		}
	}


	void insert(const Accumulator& keys, const Value& val, Int at)
	{
		if (at < size_ - 1)
		{
			insertSpace(at, 1);
		}

		for (Int c = 0; c < Blocks; c++)
		{
			key(c, at) = keys[c];
		}

		value(at) = val;
	}



	template <typename Comparator>
	Int find(Int block_num, const Key& k, Comparator &comparator) const
	{
		Int key_block_offset 	= getKeyBlockOffset(block_num);
		Int index_block_offset 	= getIndexKeyBlockOffset(block_num);

		if (comparator.TestMax(k, max_keyb(index_block_offset)))
		{
			return -1;
		}

		Int levels = 0;
		Int level_sizes[LEVELS_MAX];

		Int level_size = max_size_;

		do
		{
			level_size = getIndexCellsNumberFor(level_size);
			level_sizes[levels++] = level_size;
		}
		while (level_size > 1);

		Int base = 1, start = 0;

		for (Int level = levels - 2; level >= 0; level--)
		{
			Int level_size 	= level_sizes[level];
			Int end 		= start + BranchingFactor < level_size ? start + BranchingFactor : level_size;

			for (Int idx = start; idx < end; idx++)
			{
				const IndexKey& key0 = indexb(index_block_offset, base + idx);
				if (comparator.CompareIndex(k, key0))
				{
					start = idx * BranchingFactor;
					comparator.Sub(key0);
					break;
				}
			}

			base += level_size;
		}

		Int stop = (start + BranchingFactor) > size_ ? size_ : start + BranchingFactor;

		for (Int idx = start; idx < stop; idx++)
		{
			if (comparator.CompareKey(k, keyb(key_block_offset, idx)))
			{
				return idx;
			}
		}

		return -1;
	}

	template <typename Functor>
	void WalkRange(Int start, Int end, Functor& walker) const
	{
		if (end - start <= BranchingFactor * 2)
		{
			walker.WalkKeys(start, end);
		}
		else {
			Int block_start_end 	= getBlockStartEnd(start);
			Int block_end_start 	= getBlockStart(end);

			walker.WalkKeys(start, block_start_end);

			if (block_start_end < block_end_start)
			{
				Int level_size = getIndexCellsNumberFor(max_size_);
				walker.PrepareIndex();
				WalkIndexRange(start/BranchingFactor + 1, end/BranchingFactor, walker, index_size_ - level_size, level_size);
			}

			walker.WalkKeys(block_end_start, end);
		}
	}




	template <typename Walker>
	Int WalkFw(Int start, Walker& walker) const
	{
		Int block_limit 	= getBlockStartEnd(start);

		if (block_limit >= size())
		{
			return walker.WalkKeys(start, size());
		}
		else
		{
			Int limit = walker.WalkKeys(start, block_limit);
			if (limit < block_limit)
			{
				return limit;
			}
			else {
				walker.PrepareIndex();

				Int level_size 		= getIndexCellsNumberFor(max_size_);
				Int level_limit 	= getIndexCellsNumberFor(size_);
				Int last_start 		= WalkIndexFw(block_limit/BranchingFactor, walker, index_size_ - level_size, level_size, level_limit);

				Int last_start_end 	= getBlockStartEnd(last_start);

				Int last_end = last_start_end <= size()? last_start_end : size();

				return walker.WalkKeys(last_start, last_end);
			}
		}
	}


	template <typename Walker>
	Int WalkBw(Int start, Walker& walker) const
	{
		Int block_end 	= getBlockStartEndBw(start);

		if (block_end == -1)
		{
			return walker.WalkKeys(start, -1);
		}
		else
		{
			Int limit = walker.WalkKeys(start, block_end);
			if (limit > block_end)
			{
				return limit;
			}
			else {
				walker.PrepareIndex();

				Int level_size = getIndexCellsNumberFor(max_size_);
				Int last_start = WalkIndexBw(block_end/BranchingFactor, walker, index_size_ - level_size, level_size);

				Int last_start_end = getBlockStartEndBw(last_start);

				return walker.WalkKeys(last_start, last_start_end);
			}
		}
	}


private:

	template <typename Functor>
	void WalkIndexRange(Int start, Int end, Functor& walker, Int level_offet, Int level_size) const
	{
		if (end - start <= BranchingFactor*2)
		{
			walker.WalkIndex(start + level_offet, end + level_offet);
		}
		else {
			Int block_start_end 	= getBlockStartEnd(start);
			Int block_end_start 	= getBlockStart(end);

			walker.WalkIndex(start + level_offet, block_start_end + level_offet);

			if (block_start_end < block_end_start)
			{
				Int level_size0 = getIndexCellsNumberFor(level_size);
				WalkIndexRange(start/BranchingFactor + 1, end/BranchingFactor, walker, level_offet - level_size0, level_size0);
			}

			walker.WalkIndex(block_end_start + level_offet, end + level_offet);
		}
	}


	template <typename Walker>
	Int WalkIndexFw(Int start, Walker& walker, Int level_offet, Int level_size, Int level_limit) const
	{
		Int block_start_end 	= getBlockStartEnd(start);

		if (block_start_end >= level_limit)
		{
			return (walker.WalkIndex(start + level_offet, level_limit + level_offet) - level_offet) * BranchingFactor;
		}
		else
		{
			Int limit = walker.WalkIndex(start + level_offet, block_start_end + level_offet) - level_offet;
			if (limit < block_start_end)
			{
				return limit * BranchingFactor;
			}
			else {
				Int level_size0 	= getIndexCellsNumberFor(level_size);
				Int level_limit0 	= getIndexCellsNumberFor(level_limit);

				Int last_start  	= WalkIndexFw(block_start_end/BranchingFactor, walker, level_offet - level_size0, level_size0, level_limit0);

				Int last_start_end 	= getBlockStartEnd(last_start);

				Int last_end = last_start_end <= level_limit ? last_start_end : level_limit;

				return (walker.WalkIndex(last_start + level_offet, last_end + level_offet) - level_offet) * BranchingFactor;
			}
		}
	}

	template <typename Walker>
	Int WalkIndexBw(Int start, Walker& walker, Int level_offet, Int level_size) const
	{
		Int block_start_end 	= getBlockStartEndBw(start);

		if (block_start_end == -1)
		{
			return (walker.WalkIndex(start + level_offet, level_offet - 1) - level_offet + 1) * BranchingFactor - 1;
		}
		else
		{
			Int idx = walker.WalkIndex(start + level_offet, block_start_end + level_offet) - level_offet;
			if (idx > block_start_end)
			{
				return (idx + 1) * BranchingFactor - 1;
			}
			else {
				Int level_size0 = getIndexCellsNumberFor(level_size);
				Int last_start  = WalkIndexBw(block_start_end/BranchingFactor, walker, level_offet - level_size0, level_size0);

				Int last_start_end = getBlockStartEndBw(last_start);

				return (walker.WalkIndex(last_start + level_offet, last_start_end + level_offet) - level_offet + 1) * BranchingFactor - 1;
			}
		}
	}



protected:
	static Int getBlockStart(Int i)
	{
		return (i / BranchingFactor) * BranchingFactor;
	}

	static Int getBlockStartEnd(Int i)
	{
		return (i / BranchingFactor + 1) * BranchingFactor;
	}

	static Int getBlockStartEndBw(Int i)
	{
		return (i / BranchingFactor) * BranchingFactor - 1;
	}

	static Int getBlockEnd(Int i)
	{
		return (i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0)) * BranchingFactor;
	}

	static Int getIndexCellsNumberFor(Int i)
	{
		return i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0);
	}

private:

	void CopyData(Byte* target_memory_block, Int offset, Int new_offset, Int item_size) const
	{
		CopyBuffer(
				memory_block_ 		+ offset,
				target_memory_block + new_offset,
				size_ * item_size
		);
	}

	void CopyIndex(Byte* target_memory_block, Int offset, Int new_offset, Int item_size) const
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

	static Int getBlockSize(Int item_count)
	{
		Int key_size  = sizeof(IndexKey) * Blocks;
		Int item_size = sizeof(Key) * Blocks + getValueSize();

		return getIndexSize(item_count) * key_size + item_count * item_size;
	}

	static Int getIndexSize(Int csize)
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

	static Int getLevelsForSize(Int csize)
	{
		Int nlevels;
		for (nlevels = 0; csize > 1; nlevels++)
		{
			Int idx = csize / BranchingFactor;

			csize = ((csize % BranchingFactor) == 0) ? idx : idx + 1;
		}

		return nlevels;
	}

	static Int getParentNodesFor(Int n)
	{
		Int idx = n / BranchingFactor;

		return (n % BranchingFactor) == 0 ? idx : idx + 1;
	}


	static Int getMaxSize(Int block_size)
	{
		Int item_size 	= sizeof(Key) * Blocks + getValueSize();

		Int first 		= 1;
		Int last  		= block_size / item_size;

		while (first < last - 1)
		{
			Int middle = (first + last) / 2;

			Int size = getBlockSize(middle);
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

		if (getBlockSize(last) <= block_size)
		{
			max_size = last;
		}
		else if (getBlockSize((first + last) / 2) <= block_size)
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
