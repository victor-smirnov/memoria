
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PMAP_TREE_WALKERS_HPP_
#define MEMORIA_CORE_PMAP_TREE_WALKERS_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/traits.hpp>

#include <memoria/core/tools/bitmap.hpp>


namespace memoria {


template <typename Type>
class ComparatorBase {
protected:
	Type sum_;
public:
	ComparatorBase(): sum_(0) {}

	void Sub(const Type& k)
	{
		sum_ -= k;
	}

	Type sum() const {
		return sum_;
	}
};


template <typename Key, typename IndexKey>
class LESumComparator: public ComparatorBase<IndexKey> {
	typedef ComparatorBase<IndexKey> Base;

public:
	LESumComparator():Base() {}

	bool TestMax(const Key& k, const IndexKey& max) const
	{
		return k > max;
	}

	bool CompareIndex(const Key& k, const IndexKey& index)
	{
		Base::sum_ += index;
		return k <= Base::sum_;
	}

	bool CompareKey(const Key& k, const Key& index)
	{
		Base::sum_ += index;
		return k <= Base::sum_;
	}
};

template <typename Key, typename IndexKey>
class LTSumComparator: public ComparatorBase<IndexKey> {
	typedef ComparatorBase<IndexKey> Base;
public:
	LTSumComparator():Base() {}

	bool TestMax(const Key& k, const IndexKey& max) const
	{
		return k >= max;
	}

	bool CompareIndex(const Key& k, const IndexKey& index)
	{
		Base::sum_ += index;
		return k < Base::sum_;
	}


	bool CompareKey(const Key& k, const Key& index)
	{
		Base::sum_ += index;
		return k < Base::sum_;
	}
};


template <typename Key, typename IndexKey>
class EQSumComparator: public ComparatorBase<IndexKey> {
	typedef ComparatorBase<IndexKey> Base;
public:
	EQSumComparator():Base() {}

	bool TestMax(const Key& k, const IndexKey& max) const
	{
		return k > max;
	}

	bool CompareIndex(const Key& k, const IndexKey& index)
	{
		Base::sum_ += index;
		return k <= Base::sum_;
	}


	bool CompareKey(const Key& k, const Key& index)
	{
		Base::sum_ += index;
		return k == Base::sum_;
	}
};


template <typename TreeType, typename Key, typename IndexKey, Int Blocks>
class SumWalker
{
	IndexKey& sum_;
	const TreeType& me_;

	Int key_block_offsets_;
	Int index_block_offsets_;

public:
	SumWalker(const TreeType& me, Int block_num, IndexKey& sum):
		sum_(sum),
		me_(me)
	{
		key_block_offsets_ 		= me.GetKeyBlockOffset(block_num);
		index_block_offsets_ 	= me.GetIndexKeyBlockOffset(block_num);
	}

	void PrepareIndex() {}

	//FIXME: move offsets[] to constructor
	void WalkKeys(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			sum_ += me_.keyb(key_block_offsets_, c);
		}
	}

	void WalkIndex(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			sum_ += me_.indexb(index_block_offsets_, c);
		}
	}

	IndexKey sum() const {
		return sum_;
	}
};


template <typename TreeType, typename Key, typename IndexKey, typename Accumulator>
class SumsWalker
{
	Accumulator& sum_;
	const TreeType& me_;

	static const Int Blocks = Accumulator::Indexes;

	Int key_block_offsets_[Blocks];
	Int index_block_offsets_[Blocks];


public:
	SumsWalker(const TreeType& me, Accumulator& sum):
		sum_(sum),
		me_(me)
	{
		for (Int c = 0; c < Blocks; c++)
		{
			key_block_offsets_[c] 	= me.GetKeyBlockOffset(c);
			index_block_offsets_[c] 	= me.GetIndexKeyBlockOffset(c);
		}
	}

	void PrepareIndex() {}

	//FIXME: move offsets[] to constructor
	void WalkKeys(Int start, Int end)
	{
		for (Int block = 0; block < Blocks; block++)
		{
			for (Int c = start; c < end; c++)
			{
				sum_[block] += me_.keyb(key_block_offsets_[block], c);
			}
		}
	}

	void WalkIndex(Int start, Int end)
	{
		for (Int block = 0; block < Blocks; block++)
		{
			for (Int c = start; c < end; c++)
			{
				sum_[block] += me_.indexb(index_block_offsets_[block], c);
			}
		}
	}

	const Accumulator& sum() const {
		return sum_;
	}

	Accumulator& sum() {
		return sum_;
	}
};


template <typename TreeType, typename Key, typename IndexKey, Int Blocks>
class FindSumPositionFwFn
{
	IndexKey sum_;
	const TreeType& me_;
	Int block_num_;
	BigInt limit_;

	Int key_block_offsets_[Blocks];
	Int index_block_offsets_[Blocks];

public:
	FindSumPositionFwFn(const TreeType& me, Int block_num, BigInt limit):
		sum_(0),
		me_(me),
		block_num_(block_num),
		limit_(limit)
	{
		for (Int c = 0; c < Blocks; c++)
		{
			key_block_offsets_[c] 	= me.GetKeyBlockOffset(c);
			index_block_offsets_[c] = me.GetIndexKeyBlockOffset(c);
		}
	}

	void PrepareIndex() {}

	//FIXME: move offsets[] to constructor
	Int WalkKeys(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			IndexKey key = me_.keyb(key_block_offsets_[block_num_], c);
			IndexKey sum = sum_ + key;

			if (sum <= limit_)
			{
				sum_ = sum;
			}
			else {
				return c;
			}
		}

		return end;
	}

	Int WalkIndex(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			IndexKey sum = sum_ + me_.indexb(index_block_offsets_[block_num_], c);

			if (sum <= limit_)
			{
				sum_ = sum;
			}
			else {
				return c;
			}
		}

		return end;
	}


	IndexKey sum() const {
		return sum_;
	}
};


template <typename TreeType, typename Key, typename IndexKey, Int Blocks>
class FindSumPositionBwFn
{
	IndexKey sum_;
	const TreeType& me_;
	Int block_num_;
	BigInt limit_;

	Int key_block_offsets_[Blocks];
	Int index_block_offsets_[Blocks];

public:
	FindSumPositionBwFn(const TreeType& me, Int block_num, BigInt limit):
		sum_(0),
		me_(me),
		block_num_(block_num),
		limit_(limit)
	{
		for (Int c = 0; c < Blocks; c++)
		{
			key_block_offsets_[c] 	= me.GetKeyBlockOffset(c);
			index_block_offsets_[c] = me.GetIndexKeyBlockOffset(c);
		}
	}

	void PrepareIndex() {}

	//FIXME: move offsets[] to constructor
	Int WalkKeys(Int start, Int end)
	{
		for (Int c = start; c > end; c--)
		{
			IndexKey key = me_.keyb(key_block_offsets_[block_num_], c);
			IndexKey sum = sum_ + key;

			if (sum <= limit_)
			{
				sum_ = sum;
			}
			else {
				return c;
			}
		}

		return end;
	}

	Int WalkIndex(Int start, Int end)
	{
		for (Int c = start; c > end; c--)
		{
			IndexKey sum = sum_ + me_.indexb(index_block_offsets_[block_num_], c);

			if (sum <= limit_)
			{
				sum_ = sum;
			}
			else {
				return c;
			}
		}

		return end;
	}


	IndexKey sum() const {
		return sum_;
	}
};


template <typename TreeType, typename Key, typename IndexKey, Int Blocks>
class FindSumPositionBwLTFn
{
	IndexKey sum_;
	const TreeType& me_;
	Int block_num_;
	BigInt limit_;

	Int key_block_offsets_[Blocks];
	Int index_block_offsets_[Blocks];

public:
	FindSumPositionBwLTFn(const TreeType& me, Int block_num, BigInt limit):
		sum_(0),
		me_(me),
		block_num_(block_num),
		limit_(limit)
	{
		for (Int c = 0; c < Blocks; c++)
		{
			key_block_offsets_[c] 	= me.GetKeyBlockOffset(c);
			index_block_offsets_[c] = me.GetIndexKeyBlockOffset(c);
		}
	}

	void PrepareIndex() {}

	//FIXME: move offsets[] to constructor
	Int WalkKeys(Int start, Int end)
	{
		for (Int c = start; c > end; c--)
		{
			IndexKey key = me_.keyb(key_block_offsets_[block_num_], c);
			IndexKey sum = sum_ + key;

			if (sum < limit_)
			{
				sum_ = sum;
			}
			else {
				return c;
			}
		}

		return end;
	}

	Int WalkIndex(Int start, Int end)
	{
		for (Int c = start; c > end; c--)
		{
			IndexKey sum = sum_ + me_.indexb(index_block_offsets_[block_num_], c);

			if (sum < limit_)
			{
				sum_ = sum;
			}
			else {
				return c;
			}
		}

		return end;
	}


	IndexKey sum() const {
		return sum_;
	}
};



}


#endif