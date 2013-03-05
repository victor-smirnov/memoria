
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
#include <memoria/core/tools/bitmap_select.hpp>

#include <functional>

namespace memoria {

using namespace std;

template <typename TreeType, typename Key, typename IndexKey, Int Blocks>
class SumWalker
{
    IndexKey& sum_;

    const IndexKey* indexes_;
    const Key*		keys_;

public:
    SumWalker(const TreeType& me, Int block_num, IndexKey& sum):
        sum_(sum)
    {
        keys_ = me.keys(block_num);
        indexes_ = me.indexes(block_num);
    }

    void prepareIndex() {}
    void finish() {}

    void walkKeys(Int start, Int end)
    {
        for (Int c = start; c < end; c++)
        {
            sum_ += keys_[c];
        }
    }

    void walkIndex(Int start, Int end)
    {
        for (Int c = start; c < end; c++)
        {
            sum_ += indexes_[c];
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

    static const Int Blocks = Accumulator::Indexes;

    const Key* 		keys_[Blocks];
    const IndexKey* indexes_[Blocks];


public:
    SumsWalker(const TreeType& me, Accumulator& sum):
        sum_(sum)
    {
        for (Int c = 0; c < Blocks; c++)
        {
            keys_[c]   = me.keys(c);
            indexes_[c] = me.indexes(c);
        }
    }

    void prepareIndex() {}
    void finish() {}

    void walkKeys(Int start, Int end)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            for (Int c = start; c < end; c++)
            {
                sum_[block] += keys_[block][c];
            }
        }
    }

    void walkIndex(Int start, Int end)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            for (Int c = start; c < end; c++)
            {
                sum_[block] += indexes_[block][c];
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

template <typename K1, typename K2>
struct SumCompareLE {
	bool operator()(K1 k1, K2 k2) {
		return k1 <= k2;
	}
};

template <typename K1, typename K2>
struct SumCompareLT {
	bool operator()(K1 k1, K2 k2) {
		return k1 < k2;
	}
};


template <
	typename TreeType,
	typename IndexKey,
	template <typename, typename> class Comparator
>
class FindSumPositionFwFnBase
{
protected:
	IndexKey sum_;
    const TreeType& me_;
    BigInt limit_;

    const IndexKey* indexes_;

public:
    FindSumPositionFwFnBase(const TreeType& me, Int block_num, BigInt limit):
        sum_(0),
        me_(me),
        limit_(limit)
    {
    	indexes_ = me_.indexes(block_num);
    }

    void prepareIndex() {}
    void finish() {}

    Int walkIndex(Int start, Int end)
    {
        Comparator<BigInt, IndexKey> compare;

    	for (Int c = start; c < end; c++)
        {
        	IndexKey key = indexes_[c];

        	if (compare(key, limit_))
        	{
        		sum_ 	+= key;
        		limit_ 	-= key;
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



template <
	typename TreeType,
	template <typename, typename> class Comparator
>
class FindSumPositionFwAccFnBase
{
protected:
	typedef typename TreeType::IndexKey 	IndexKey;
	typedef typename TreeType::Accumulator 	Accumulator;

    static const Int Blocks = TreeType::Blocks;

	Accumulator sum_;
    const TreeType& me_;
    BigInt limit_;

    const IndexKey* indexes_;

public:
    FindSumPositionFwAccFnBase(const TreeType& me, Int block_num, BigInt limit):
        sum_(),
        me_(me),
        limit_(limit)
    {
    	indexes_ = me_.indexes(block_num);
    }

    void prepareIndex() {}
    void finish() {}

    void sumIndex(Int from, Int to)
    {
    	for (Int block = 0; block < Blocks; block++)
    	{
    		const IndexKey* indexes = me_.indexes(block);

    		for (Int c = from; c != to; c++)
    		{
    			sum_[block] += indexes[c];
    		}
    	}
    }

    Int walkIndex(Int start, Int end)
    {
        Comparator<BigInt, IndexKey> compare;

    	for (Int c = start; c < end; c++)
        {
        	IndexKey key = indexes_[c];

        	if (compare(key, limit_))
        	{
        		limit_ 	-= key;
        	}
        	else {
        		sumIndex(start, c);
        		return c;
        	}
        }

    	sumIndex(start, end);

        return end;
    }


    Accumulator sum() const
    {
        return sum_;
    }
};




template <
	typename TreeType,
	typename Key,
	typename IndexKey,
	template <typename, typename> class Comparator>
class FindSumPositionFwFn: public FindSumPositionFwFnBase<TreeType, IndexKey, Comparator>
{
	typedef FindSumPositionFwFnBase<TreeType, IndexKey, Comparator> Base;
	const Key* keys_;

public:
    FindSumPositionFwFn(const TreeType& me, Int block_num, BigInt limit):
        Base(me, block_num, limit)
    {
    	keys_ 	 = me.keys(block_num);
    }

    void prepareIndex() {}

    Int walkKeys(Int start, Int end)
    {
    	Comparator<BigInt, IndexKey> compare;

    	for (Int c = start; c < end; c++)
        {
            IndexKey key = keys_[c];

            if (compare(key, Base::limit_))
            {
            	Base::sum_ 		+= key;
            	Base::limit_ 	-= key;
            }
            else {
            	return c;
            }
        }

        return end;
    }
};



template <
	typename TreeType,
	template <typename, typename> class Comparator
>
class FindSumPositionFwCompoundFn
{
protected:

	typedef typename TreeType::Key 			Key;
	typedef typename TreeType::IndexKey 	IndexKey;
	typedef typename TreeType::Accumulator 	Accumulator;

    const TreeType& 	me_;

    Int 				blocks_count_;
    const Int* 			blocks_;
    BigInt* 			sums_;
    BigInt 				limit_;

public:
    FindSumPositionFwCompoundFn(
    		const TreeType& me,
    		Int blocks_count,
    		const Int* blocks,
    		BigInt* sums,
    		BigInt limit
    ):
        me_(me),
        blocks_count_(blocks_count),
        blocks_(blocks),
        sums_(sums),
        limit_(limit)
    {}

    void prepareIndex() {}
    void finish() {}

    void sumIndex(Int from, Int to)
    {
    	for (Int block = 0; block < blocks_count_; block++)
    	{
    		const IndexKey* indexes = me_.indexes(blocks_[block]);

    		for (Int c = from; c < to; c++)
    		{
    			sums_[block] += indexes[c];
    		}
    	}
    }

    Int walkIndex(Int start, Int end)
    {
        Comparator<BigInt, IndexKey> compare;

        const IndexKey* indexes = me_.indexes(blocks_[0]);

    	for (Int c = start; c < end; c++)
        {
        	IndexKey key = indexes[c];

        	if (compare(key, limit_))
        	{
        		limit_ 	-= key;
        	}
        	else {
        		sumIndex(start, c);
        		return c;
        	}
        }

    	sumIndex(start, end);

        return end;
    }

    void sumKeys(Int from, Int to)
    {
    	for (Int block = 0; block < blocks_count_; block++)
    	{
    		const Key* keys = me_.keys(blocks_[block]);

    		for (Int c = from; c < to; c++)
    		{
    			sums_[block] += keys[c];
    		}
    	}
    }


    Int walkKeys(Int start, Int end)
    {
    	Comparator<BigInt, IndexKey> compare;

    	const Key* keys = me_.keys(blocks_[0]);

    	for (Int c = start; c < end; c++)
    	{
    		IndexKey key = keys[c];

    		if (compare(key, limit_))
    		{
    			limit_ 	-= key;
    		}
    		else {
    			sumKeys(start, c);
    			return c;
    		}
    	}

    	sumKeys(start, end);

    	return end;
    }
};


template <
	typename TreeType,
	template <typename, typename> class Comparator
>
class FindSumPositionBwCompoundFn
{
protected:

	typedef typename TreeType::Key 			Key;
	typedef typename TreeType::IndexKey 	IndexKey;
	typedef typename TreeType::Accumulator 	Accumulator;

    const TreeType& 	me_;

    Int 				blocks_count_;
    const Int* 			blocks_;
    BigInt* 			sums_;
    BigInt 				limit_;

public:
    FindSumPositionBwCompoundFn(
    		const TreeType& me,
    		Int blocks_count,
    		const Int* blocks,
    		BigInt* sums,
    		BigInt limit
    ):
        me_(me),
        blocks_count_(blocks_count),
        blocks_(blocks),
        sums_(sums),
        limit_(limit)
    {}

    void prepareIndex() {}
    void finish() {}

    void sumIndex(Int from, Int to)
    {
    	for (Int block = 0; block < blocks_count_; block++)
    	{
    		const IndexKey* indexes = me_.indexes(blocks_[block]);

    		for (Int c = from; c > to; c--)
    		{
    			sums_[block] += indexes[c];
    		}
    	}
    }

    Int walkIndex(Int start, Int end)
    {
        Comparator<BigInt, IndexKey> compare;

        const IndexKey* indexes = me_.indexes(blocks_[0]);

    	for (Int c = start; c > end; c--)
        {
        	IndexKey key = indexes[c];

        	if (compare(key, limit_))
        	{
        		limit_ 	-= key;
        	}
        	else {
        		sumIndex(start, c);
        		return c;
        	}
        }

    	sumIndex(start, end);

        return end;
    }

    void sumKeys(Int from, Int to)
    {
    	for (Int block = 0; block < blocks_count_; block++)
    	{
    		const Key* keys = me_.keys(blocks_[block]);

    		for (Int c = from; c > to; c--)
    		{
    			sums_[block] += keys[c];
    		}
    	}
    }


    Int walkKeys(Int start, Int end)
    {
    	Comparator<BigInt, IndexKey> compare;

    	const Key* keys = me_.keys(blocks_[0]);

    	for (Int c = start; c > end; c--)
    	{
    		IndexKey key = keys[c];

    		if (compare(key, limit_))
    		{
    			limit_ 	-= key;
    		}
    		else {
    			sumKeys(start, c);
    			return c;
    		}
    	}

    	sumKeys(start, end);

    	return end;
    }
};


template <
	typename TreeType,
	template <typename, typename> class Comparator
>
class FindSumPositionFwAccFn: public FindSumPositionFwAccFnBase<TreeType, Comparator>
{
	typedef FindSumPositionFwAccFnBase<TreeType, Comparator> 	Base;
	typedef typename TreeType::Key 								Key;
	typedef typename TreeType::IndexKey 						IndexKey;

	const Key* keys_;

public:
    FindSumPositionFwAccFn(const TreeType& me, Int block_num, BigInt limit):
        Base(me, block_num, limit)
    {
    	keys_ 	 = me.keys(block_num);
    }

    void prepareIndex() {}
    void finish() {}

    void sumKeys(Int from, Int to)
    {
    	for (Int block = 0; block < Base::Blocks; block++)
    	{
    		const Key* keys = Base::me_.keys(block);

    		for (Int c = from; c != to; c++)
    		{
    			Base::sum_[block] += keys[c];
    		}
    	}
    }

    Int walkKeys(Int start, Int end)
    {
    	Comparator<BigInt, IndexKey> compare;

    	for (Int c = start; c < end; c++)
        {
            IndexKey key = keys_[c];

            if (compare(key, Base::limit_))
            {
            	Base::limit_ 	-= key;
            }
            else {
            	sumKeys(start, c);
            	return c;
            }
        }

    	sumKeys(start, end);

        return end;
    }
};



template <
	typename TreeType,
	typename IndexKey,
	template <typename, typename> class Comparator
>
class FindSumPositionBwFnBase
{
protected:
    IndexKey sum_;
    const TreeType& me_;
    BigInt limit_;

    const IndexKey* indexes_;

public:
    FindSumPositionBwFnBase(const TreeType& me, Int block_num, BigInt limit):
        sum_(0),
        me_(me),
        limit_(limit)
    {
    	indexes_ = me.indexes(block_num);
    }

    void prepareIndex() {}
    void finish() {}

    Int walkIndex(Int start, Int end)
    {
        Comparator<BigInt, IndexKey> comparator;

    	for (Int c = start; c > end; c--)
        {
            IndexKey sum = indexes_[c];

            if (comparator(sum, limit_))
            {
                sum_ 	+= sum;
                limit_	-= sum;
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


template <
	typename TreeType,
	template <typename, typename> class Comparator
>
class FindSumPositionBwAccFnBase
{
protected:
    typedef typename TreeType::IndexKey 	IndexKey;
    typedef typename TreeType::Accumulator 	Accumulator;

    static const Int Blocks = TreeType::Blocks;

	const TreeType& me_;

	Int block_num_;
	BigInt limit_;
    Accumulator sum_;

    const IndexKey* indexes_;

public:
    FindSumPositionBwAccFnBase(const TreeType& me, Int block_num, BigInt limit):
        me_(me),
        block_num_(block_num),
        limit_(limit),
        sum_()
    {
    	indexes_ = me.indexes(block_num);
    }

    void prepareIndex() {}
    void finish() {}

    void sumIndex(Int from, Int to)
    {
    	for (Int block = 0; block < Blocks; block++)
    	{
    		const IndexKey* indexes = me_.indexes(block);

    		for (Int c = from; c != to; c--)
    		{
    			sum_[block] += indexes[c];
    		}
    	}
    }

    Int walkIndex(Int start, Int end)
    {
        Comparator<BigInt, IndexKey> comparator;

    	for (Int c = start; c > end; c--)
        {
            IndexKey sum = indexes_[c];

            if (comparator(sum, limit_))
            {
                limit_ -= sum;
            }
            else {
            	sumIndex(start, c);
            	return c;
            }
        }

    	sumIndex(start, end);
        return end;
    }


    Accumulator sum() const {
        return sum_;
    }
};





template <
	typename TreeType,
	typename Key,
	typename IndexKey,
	template <typename, typename> class Comparator
>
class FindSumPositionBwFn: public FindSumPositionBwFnBase<TreeType, IndexKey, Comparator>
{
	typedef FindSumPositionBwFnBase<TreeType, IndexKey, Comparator> Base;


    const IndexKey* keys_;

public:
    FindSumPositionBwFn(const TreeType& me, Int block_num, BigInt limit):
        Base(me, block_num, limit)
    {
    	keys_  = me.keys(block_num);
    }

    void prepareIndex() {}

    Int walkKeys(Int start, Int end)
    {
    	Comparator<Key, IndexKey> comparator;

        for (Int c = start; c > end; c--)
        {
            IndexKey key = keys_[c];

            if (comparator(key, Base::limit_))
            {
                Base::sum_ 		+= key;
                Base::limit_ 	-= key;
            }
            else {
                return c;
            }
        }

        return end;
    }

};

template <typename TreeType, typename Key, typename IndexKey>
using FindSumPositionFwLtFn = FindSumPositionFwFn<TreeType, Key, IndexKey, SumCompareLE>;

template <typename TreeType, typename Key, typename IndexKey>
using FindSumPositionFwLeFn = FindSumPositionFwFn<TreeType, Key, IndexKey, SumCompareLT>;

template <typename TreeType, typename Key, typename IndexKey>
using FindSumPositionBwLtFn = FindSumPositionBwFn<TreeType, Key, IndexKey, SumCompareLE>;

template <typename TreeType, typename Key, typename IndexKey>
using FindSumPositionBwLEFn = FindSumPositionBwFn<TreeType, Key, IndexKey, SumCompareLT>;


template <typename TreeType>
using FindSumPositionFwAccLtFn = FindSumPositionFwAccFn<TreeType, SumCompareLE>;

template <typename TreeType>
using FindSumPositionFwAccLeFn = FindSumPositionFwAccFn<TreeType, SumCompareLT>;


template <typename TreeType, Int Bits>
class RankWalker {
	typedef typename TreeType::IndexKey IndexKey;
	typedef typename TreeType::Value 	Value;

	IndexKey 		sum_;
	const TreeType& me_;
	Value 			symbol_;

	static const Int Blocks = TreeType::Blocks;

	const IndexKey* indexes_;

public:
	RankWalker(const TreeType& me, Value symbol, IndexKey sum = 0):
		sum_(sum),
		me_(me),
		symbol_(symbol)
	{
		indexes_ = me.indexes(symbol);
	}

	void prepareIndex() {}
	void finish() {}

	void walkValues(Int start, Int end)
	{
		sum_ += me_.popCount(start, end, symbol_);
	}

	void walkIndex(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			sum_ += indexes_[c];
		}
	}

	IndexKey sum() const
	{
		return sum_;
	}
};


template <typename TreeType>
class RankWalker<TreeType, 1>
{
    typedef typename TreeType::IndexKey IndexKey;
    typedef typename TreeType::Value 	Value;

    IndexKey 		sum_;
    const TreeType& me_;
    Value 			symbol_;

    static const Int Blocks = TreeType::Blocks;

    const Value* 	values_;
    const IndexKey* indexes_;

public:
    RankWalker(const TreeType& me, Value symbol, IndexKey sum = 0):
        sum_(sum),
        me_(me),
        symbol_(symbol)
    {
    	values_ 	= me.valuesBlock();
    	indexes_ 	= me.indexes(symbol);
    }

    void prepareIndex() {}
    void finish() {}

    void walkValues(Int start, Int end)
    {
    	size_t count = PopCount(values_, start, end);

    	if (symbol_)
    	{
    		sum_ += count;
    	}
    	else {
    		sum_ += end - start - count;
    	}
    }

    void walkIndex(Int start, Int end)
    {
    	for (Int c = start; c < end; c++)
    	{
    		IndexKey count = indexes_[c];
    		sum_ += count;
    	}
    }

    IndexKey sum() const
    {
    	return sum_;
    }
};






template <
	typename TreeType,
	typename MyType,
	typename IndexKey,
	template <typename, typename> class Comparator
>
class FindForwardFnBase
{
protected:
	IndexKey sum_;

    BigInt limit_;

    const IndexKey* indexes_;

public:
    FindForwardFnBase(const IndexKey* indexes, BigInt limit):
        sum_(0),
        limit_(limit),
        indexes_(indexes)
    {}

    void prepareIndex() {}
    void finish() {}

    Int walkIndex(Int start, Int end, Int size)
    {
        Comparator<BigInt, IndexKey> compare;

    	for (Int c = start; c < end; c++)
        {
        	IndexKey key = indexes_[c];

        	if (compare(key, limit_))
        	{
        		sum_ 	+= key;
        		limit_ 	-= key;
        	}
        	else {
        		me().processIndexes(start, c);
        		return c;
        	}
        }

    	me().processIndexes(start, end);
        return end;
    }


    IndexKey sum() const {
        return sum_;
    }

    MyType& me() {
    	return *static_cast<MyType*>(this);
    }
};

template <typename K1, typename K2>
struct VLECompareLE {
	bool operator()(K1 k1, K2 k2) {
		return k1 <= k2;
	}
};

template <typename K1, typename K2>
struct VLECompareLT {
	bool operator()(K1 k1, K2 k2) {
		return k1 < k2;
	}
};



template <typename TreeType, typename MyType>
class GetValueOffsetFnBase: public FindForwardFnBase<TreeType, MyType, typename TreeType::IndexKey, VLECompareLE> {

	typedef FindForwardFnBase<TreeType, MyType, typename TreeType::IndexKey, VLECompareLE> 	Base;

protected:

	typedef typename TreeType::Value 		Value;
	typedef typename TreeType::Codec 		Codec;

public:
	static const Int BranchingFactor        = TreeType::BranchingFactor;
	static const Int ValuesPerBranch        = TreeType::ValuesPerBranch;
private:

	const TreeType& me_;

	const UByte* values_;

public:
	GetValueOffsetFnBase(const TreeType& me, Int limit): Base(me.sizes(), limit), me_(me)
	{
		values_  = me.getValues();
	}

	Int max_size() const {
		return me_.max_size();
	}

	Int size() const {
		return me_.max_size();
	}


	Int walkLastValuesBlock(Int value_block_num)
	{
		Int offset = me_.offset(value_block_num);

		Int pos = value_block_num * ValuesPerBranch + offset;
		Int end = me_.max_size();

		VLECompareLE<Int, BigInt> compare;
		Codec codec;

		while (pos < end)
		{
			Int value = pos < end;

			if (compare(value, Base::limit_))
			{
				Base::sum_ 	 ++;
				Base::limit_ --;

				Value val;

				pos += codec.decode(values_, val, pos);

				Base::me().processValue(val);
			}
			else {
				return pos;
			}
		}

		return end;
	}
};





}


#endif
