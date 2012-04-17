
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PMAP_PACKED_MAP2_HPP_
#define MEMORIA_CORE_PMAP_PACKED_MAP2_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/traits.hpp>

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

	class ComparatorBase {
	protected:
		IndexKey sum_;
	public:
		ComparatorBase(): sum_(0) {}

		void Sub(const Key& k)
		{
			sum_ -= k;
		}

		IndexKey sum() const {
			return sum_;
		}
	};

	class LESumComparator: public ComparatorBase {

	public:
		LESumComparator():ComparatorBase() {}

		bool TestMax(const Key& k, const IndexKey& max) const
		{
			return k > max;
		}

		bool CompareIndex(const Key& k, const IndexKey& index)
		{
			ComparatorBase::sum_ += index;
			return k <= ComparatorBase::sum_;
		}

		bool CompareKey(const Key& k, const Key& index)
		{
			ComparatorBase::sum_ += index;
			return k <= ComparatorBase::sum_;
		}
	};

	class LTSumComparator: public ComparatorBase {
	public:
		LTSumComparator():ComparatorBase() {}

		bool TestMax(const Key& k, const IndexKey& max) const
		{
			return k >= max;
		}

		bool CompareIndex(const Key& k, const IndexKey& index)
		{
			ComparatorBase::sum_ += index;
			return k < ComparatorBase::sum_;
		}


		bool CompareKey(const Key& k, const Key& index)
		{
			ComparatorBase::sum_ += index;
			return k < ComparatorBase::sum_;
		}
	};


	class EQSumComparator: public ComparatorBase {

	public:
		EQSumComparator():ComparatorBase() {}

		bool TestMax(const Key& k, const IndexKey& max) const
		{
			return k > max;
		}

		bool CompareIndex(const Key& k, const IndexKey& index)
		{
			ComparatorBase::sum_ += index;
			return k <= ComparatorBase::sum_;
		}


		bool CompareKey(const Key& k, const Key& index)
		{
			ComparatorBase::sum_ += index;
			return k == ComparatorBase::sum_;
		}
	};

	class SumWalker
	{
		BigInt sum_;
		const MyType& me_;
	public:
		SumWalker(const MyType& me): sum_(0), me_(me) {}

		void WalkKeys(Int offsets[Blocks], Int start, Int end)
		{
			for (Int c = start; c < end; c++)
			{
				sum_ += me_.keyb(offsets[0], c);
			}
		}

		void WalkIndex(Int offsets[Blocks], Int start, Int end)
		{
			for (Int c = start; c < end; c++)
			{
				sum_ += me_.indexb(offsets[0], c);
			}
		}


		BigInt sum() const {
			return sum_;
		}
	};

private:

	static const Int LEVELS_MAX				= 32;

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
		return block_item<Value>(GetValueBlockOffset(), value_num, GetValueSize());
	}

	const Value& value(Int value_num) const
	{
		return block_item<Value>(GetValueBlockOffset(), value_num, GetValueSize());
	}

	static Int GetValueSize()
	{
		return ValueTraits<Value>::Size;
	}

	void Enlarge(Byte* target_memory_block, Int new_keys_size, Int new_index_size)
	{
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
		Int max_size 	= GetMaxSize(block_size);
		Int index_size	= GetIndexSize(max_size);

		Enlarge(memory_block_, max_size, index_size);

		max_size_ 	 	= max_size;
		index_size_	 	= index_size;
	}

	void EnlargeTo(MyType* other)
	{
		Enlarge(other->memory_block_, other->max_size_, other->index_size_);
	}

	void Shrink(Byte* target_memory_block, Int new_keys_size, Int new_index_size)
	{
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
		Int max_size 	= GetMaxSize(block_size);
		Int index_size	= GetIndexSize(max_size);

		Shrink(memory_block_, max_size, index_size);

		max_size_ 	 	= max_size;
		index_size_	 	= index_size;
	}

	void ShrinkTo(MyType* other)
	{
		Shrink(other->memory_block_, other->max_size_, other->index_size_);
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

	void ReindexAll(Int start, Int end)
	{
		for (Int c = 0; c < Blocks; c++)
		{
			Reindex(c, start, end);
		}
	}

	void Reindex(Int block_num, Int start, Int end)
	{
		Int block_start = GetBlockStart(start);
		Int block_end 	= GetBlockEnd(end);

		Int index_block_offset 	= GetIndexKeyBlockOffset(block_num);
		Int key_block_offset 	= GetKeyBlockOffset(block_num);

		Int index_level_size	= GetIndexCellsNumberFor(max_size_);
		Int index_level_start 	= index_size_ - index_level_size;

		Int level_max = size_;

		for (Int c = block_start; c < block_end; c += BranchingFactor)
		{
			IndexKey sum = 0;
			Int max 	 = c + BranchingFactor <= level_max ? c + BranchingFactor : level_max;

			for (Int d = c; d < max; d++)
			{
				sum += keyb(key_block_offset, d);
			}

			Int idx = c / BranchingFactor + index_level_start;
			indexb(index_block_offset, idx) = sum;
		}

		while (index_level_start > 0)
		{
			level_max 		= GetIndexCellsNumberFor(level_max);
			block_start 	= GetBlockStart(block_start / BranchingFactor);
			block_end 		= GetBlockEnd(block_end / BranchingFactor);

			Int index_parent_size 	= GetIndexCellsNumberFor(index_level_size);
			Int index_parent_start	= index_level_start - index_parent_size;

			for (Int c = block_start; c < block_end; c += BranchingFactor)
			{
				IndexKey sum = 0;
				Int max 	 = (c + BranchingFactor <= level_max ? c + BranchingFactor : level_max) + index_level_start;

				for (Int d = c + index_level_start; d < max; d++)
				{
					sum += indexb(index_block_offset, d);
				}

				Int idx = c / BranchingFactor + index_parent_start;
				indexb(index_block_offset, idx) = sum;
			}

			index_level_size 	= index_parent_size;
			index_level_start 	-= index_parent_size;
		}
	}

	void Add(Int block_num, Int idx, const Key& value)
	{
		key(block_num, idx) += value;

		Int index_block_offset 	= GetIndexKeyBlockOffset(block_num);

		Int index_level_size	= GetIndexCellsNumberFor(max_size_);
		Int index_level_start 	= index_size_ - index_level_size;

		while (index_level_start >= 0)
		{
			idx /= BranchingFactor;

			indexb(index_block_offset, idx + index_level_start) += value;

			Int index_parent_size 	= GetIndexCellsNumberFor(index_level_size);

			index_level_size 		= index_parent_size;
			index_level_start 		-= index_parent_size;
		}
	}


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

	template <typename Comparator>
	Int Find(Int block_num, const Key& k, Comparator &comparator) const
	{
		Int key_block_offset 	= GetKeyBlockOffset(block_num);
		Int index_block_offset 	= GetIndexKeyBlockOffset(block_num);

		if (comparator.TestMax(k, max_keyb(index_block_offset)))
		{
			return -1;
		}

		Int levels = 0;
		Int level_sizes[LEVELS_MAX];

		Int level_size = max_size_;

		do
		{
			level_size = GetIndexCellsNumberFor(level_size);
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

	Int FindLE(Int block_num, const Key& k) const
	{
		LESumComparator cmp;
		return Find(block_num, k, cmp);
	}

	Int FindLE(Int block_num, const Key& k, Accumulator& acc) const
	{
		LESumComparator cmp;
		Int result = Find(block_num, k, cmp);
		acc[block_num] += cmp.sum();
		return result;
	}



	Int FindLT(Int block_num, const Key& k) const
	{
		LTSumComparator cmp;
		return Find(block_num, k, cmp);
	}

	Int FindLT(Int block_num, const Key& k, Accumulator& acc) const
	{
		LTSumComparator cmp;
		Int result = Find(block_num, k, cmp);
		acc[block_num] += cmp.sum();
		return result;
	}



	Int FindEQ(Int block_num, const Key& k) const
	{
		EQSumComparator cmp;
		return Find(block_num, k, cmp);
	}

	Int FindEQ(Int block_num, const Key& k, Accumulator& acc) const
	{
		EQSumComparator cmp;
		Int result = Find(block_num, k, cmp);
		acc[block_num] += cmp.sum();
		return result;
	}

	template <typename Functor>
	void Walk(Int block_num, Int start, Int end, Functor& walker) const
	{
		Int keys_offsets[Blocks];

		for (Int c = 0; c < Blocks; c++)
		{
			keys_offsets[c] = GetKeyBlockOffset(block_num);
		}

		if (end - start <= BranchingFactor * 2)
		{
			walker.WalkKeys(keys_offsets, start, end);
		}
		else {
			Int block_start_end 	= GetBlockStartEnd(start);
			Int block_end_start 	= GetBlockStart(end);

			walker.WalkKeys(keys_offsets, start, block_start_end);

			if (block_start_end < block_end_start)
			{
				Int index_offsets[Blocks];

				for (Int c = 0; c < Blocks; c++)
				{
					index_offsets[c] = GetIndexKeyBlockOffset(block_num);
				}

				Int level_size = GetIndexCellsNumberFor(max_size_);
				WalkIndex(index_offsets, start/BranchingFactor + 1, end/BranchingFactor, walker, index_size_ - level_size, level_size);
			}

			walker.WalkKeys(keys_offsets, block_end_start, end);
		}
	}



	void Sum(Int block_num, Int start, Int end, Accumulator& accum) const
	{
		SumWalker walker(*this);
		Walk(block_num, start, end, walker);
		accum[block_num] += walker.sum();
	}

private:

	template <typename Functor>
	void WalkIndex(Int index_offsets[Blocks], Int start, Int end, Functor& walker, Int level_offet, Int level_size) const
	{
		if (end - start <= BranchingFactor*2)
		{
			walker.WalkIndex(index_offsets, start + level_offet, end + level_offet);
		}
		else {
			Int block_start_end 	= GetBlockStartEnd(start);
			Int block_end_start 	= GetBlockStart(end);

			walker.WalkIndex(index_offsets, start + level_offet, block_start_end + level_offet);

			if (block_start_end < block_end_start)
			{
				Int level_size0 = GetIndexCellsNumberFor(level_size);
				WalkIndex(index_offsets, start/BranchingFactor + 1, end/BranchingFactor, walker, level_offet - level_size0, level_size0);
			}

			walker.WalkIndex(index_offsets, block_end_start + level_offet, end + level_offet);
		}
	}


	static Int GetBlockStart(Int i)
	{
		return (i / BranchingFactor) * BranchingFactor;
	}
	static Int GetBlockStartEnd(Int i)
	{
		return (i / BranchingFactor + 1) * BranchingFactor;
	}

	static Int GetBlockEnd(Int i)
	{
		return (i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0)) * BranchingFactor;
	}

	static Int GetIndexCellsNumberFor(Int i)
	{
		return i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0);
	}

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

	static Int GetBlockSize(Int item_count)
	{
		Int key_size  = sizeof(IndexKey) * Blocks;
		Int item_size = sizeof(Key) * Blocks + GetValueSize();

		return GetIndexSize(item_count) * key_size + item_count * item_size;
	}

	static Int GetIndexSize(Int csize)
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

	static Int GetLevelsForSize(Int csize)
	{
		Int nlevels;
		for (nlevels = 0; csize > 1; nlevels++)
		{
			Int idx = csize / BranchingFactor;

			csize = ((csize % BranchingFactor) == 0) ? idx : idx + 1;
		}

		return nlevels;
	}

	static Int GetParentNodesFor(Int n)
	{
		Int idx = n / BranchingFactor;

		return (n % BranchingFactor) == 0 ? idx : idx + 1;
	}


	static Int GetMaxSize(Int block_size)
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
