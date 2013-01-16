
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

    bool testMax(const Key& k, const IndexKey& max) const
    {
        return k > max;
    }

    bool compareIndex(const Key& k, const IndexKey& index)
    {
        Base::sum_ += index;
        return k <= Base::sum_;
    }

    bool compareKey(const Key& k, const Key& index)
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

    bool testMax(const Key& k, const IndexKey& max) const
    {
        return k >= max;
    }

    bool compareIndex(const Key& k, const IndexKey& index)
    {
        Base::sum_ += index;
        return k < Base::sum_;
    }


    bool compareKey(const Key& k, const Key& index)
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

    bool testMax(const Key& k, const IndexKey& max) const
    {
        return k > max;
    }

    bool compareIndex(const Key& k, const IndexKey& index)
    {
        Base::sum_ += index;
        return k <= Base::sum_;
    }


    bool compareKey(const Key& k, const Key& index)
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
        key_block_offsets_      = me.getKeyBlockOffset(block_num);
        index_block_offsets_    = me.getIndexKeyBlockOffset(block_num);
    }

    void prepareIndex() {}

    //FIXME: move offsets[] to constructor
    void walkKeys(Int start, Int end)
    {
        for (Int c = start; c < end; c++)
        {
            sum_ += me_.keyb(key_block_offsets_, c);
        }
    }

    void walkIndex(Int start, Int end)
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
            key_block_offsets_[c]   = me.getKeyBlockOffset(c);
            index_block_offsets_[c]     = me.getIndexKeyBlockOffset(c);
        }
    }

    void prepareIndex() {}

    //FIXME: move offsets[] to constructor
    void walkKeys(Int start, Int end)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            for (Int c = start; c < end; c++)
            {
                sum_[block] += me_.keyb(key_block_offsets_[block], c);
            }
        }
    }

    void walkIndex(Int start, Int end)
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
            key_block_offsets_[c]   = me.getKeyBlockOffset(c);
            index_block_offsets_[c] = me.getIndexKeyBlockOffset(c);
        }
    }

    void prepareIndex() {}

    //FIXME: move offsets[] to constructor
    Int walkKeys(Int start, Int end)
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

    Int walkIndex(Int start, Int end)
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
            key_block_offsets_[c]   = me.getKeyBlockOffset(c);
            index_block_offsets_[c] = me.getIndexKeyBlockOffset(c);
        }
    }

    void prepareIndex() {}

    //FIXME: move offsets[] to constructor
    Int walkKeys(Int start, Int end)
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

    Int walkIndex(Int start, Int end)
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
            key_block_offsets_[c]   = me.getKeyBlockOffset(c);
            index_block_offsets_[c] = me.getIndexKeyBlockOffset(c);
        }
    }

    void prepareIndex() {}

    //FIXME: move offsets[] to constructor
    Int walkKeys(Int start, Int end)
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

    Int walkIndex(Int start, Int end)
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





template <typename TreeType, Int Bits>
class RankWalker {
	typedef typename TreeType::IndexKey IndexKey;
	typedef typename TreeType::Value 	Value;

	IndexKey 		sum_;
	const TreeType& me_;
	Value 			symbol_;

	static const Int Blocks = TreeType::Blocks;

	Int value_block_offset_;
	Int index_block_offset_;

public:
	RankWalker(const TreeType& me, Value symbol, IndexKey sum = 0):
		sum_(sum),
		me_(me),
		symbol_(symbol)
	{
		value_block_offset_ = me.getValueBlockOffset();
		index_block_offset_ = me.getIndexKeyBlockOffset(symbol);
	}

	void prepareIndex() {}

	void walkValues(Int start, Int end)
	{
		sum_ += me_.popCount(start, end, symbol_);
	}

	void walkIndex(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			sum_ += me_.indexb(index_block_offset_, c);
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

    Int value_block_offset_;
    Int index_block_offset_;

public:
    RankWalker(const TreeType& me, Value symbol, IndexKey sum = 0):
        sum_(sum),
        me_(me),
        symbol_(symbol)
    {
    	value_block_offset_ = me.getValueBlockOffset();
    	index_block_offset_ = me.getIndexKeyBlockOffset(symbol);
    }

    void prepareIndex() {}

    void walkValues(Int start, Int end)
    {
    	const Value* buffer = T2T<const Value*>(me_.memoryBlock() + value_block_offset_);
    	size_t count = PopCount(buffer, start, end);

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
    		IndexKey count = me_.indexb(index_block_offset_, c);

    		sum_ += count;
    	}
    }

    IndexKey sum() const
    {
    	return sum_;
    }
};


template <typename TreeType>
class SelectFWWalkerBase {

protected:
	typedef typename TreeType::IndexKey IndexKey;
	typedef typename TreeType::Value 	Value;

	IndexKey 		rank_;
	IndexKey 		limit_;
	const TreeType& me_;
	Value 			symbol_;

	Int index_block_offset_;

	bool			found_;

public:
	SelectFWWalkerBase(const TreeType& me, Value symbol, IndexKey limit):
		rank_(0),
		limit_(limit),
		me_(me),
		symbol_(symbol)
	{
		index_block_offset_ = me.getIndexKeyBlockOffset(symbol);
	}

	void prepareIndex() {}

	Int walkIndex(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			IndexKey block_rank = me_.indexb(index_block_offset_, c);

			if (block_rank >= limit_)
			{
				return c;
			}

			rank_  += block_rank;
			limit_ -= block_rank;
		}

		return end;
	}

	IndexKey rank() const
	{
		return rank_;
	}

	bool is_found() const
	{
		return found_;
	}
};



template <typename TreeType, Int Bits>
class SelectFWWalker;




template <typename TreeType>
class SelectFWWalker<TreeType, 1>: public SelectFWWalkerBase<TreeType> {

	typedef SelectFWWalkerBase<TreeType> Base;

	typedef typename Base::IndexKey IndexKey;
    typedef typename Base::Value 	Value;

public:
    SelectFWWalker(const TreeType& me, Value symbol, IndexKey limit):
    	Base(me, symbol, limit)
    {}


    Int walkValues(Int start, Int end)
    {
    	const Value* buffer = Base::me_.valuesBlock();

    	auto result = Base::symbol_?
    			Select1FW(buffer, start, end, Base::limit_) :
    			Select0FW(buffer, start, end, Base::limit_);

    	Base::rank_ 	+= result.rank();
    	Base::limit_  -= result.rank();

    	Base::found_	= result.is_found() || Base::limit_ == 0;

    	return result.idx();
    }
};




template <typename TreeType, Int Bits>
class SelectFWWalker: public SelectFWWalkerBase<TreeType> {

	typedef SelectFWWalkerBase<TreeType> Base;

	typedef typename Base::IndexKey IndexKey;
    typedef typename Base::Value 	Value;

    Int value_block_offset_;

public:
    SelectFWWalker(const TreeType& me, Value symbol, IndexKey limit):
    	Base(me, symbol, limit)
    {
    	value_block_offset_ = me.getValueBlockOffset();
    }

    Int walkValues(Int start, Int end)
    {
    	IndexKey total = 0;

    	for (Int c = start; c < end; c++)
    	{
    		total += Base::me_.testb(value_block_offset_, c, Base::symbol_);

    		if (total == Base::limit_)
    		{
    			Base::rank_ += total;
    			Base::found_ = true;

    			return c;
    		}
    	}

    	Base::rank_ 	+= total;
    	Base::limit_  	-= total;
    	Base::found_ 	= Base::limit_ == 0;

    	return end;
    }
};








template <typename TreeType>
class SelectBWWalkerBase {

protected:
	typedef typename TreeType::IndexKey IndexKey;
    typedef typename TreeType::Value 	Value;

    IndexKey 		rank_;
    IndexKey 		limit_;
    const TreeType& me_;
    Value 			symbol_;

    Int index_block_offset_;

    bool			found_;

public:
    SelectBWWalkerBase(const TreeType& me, Value symbol, IndexKey limit):
        rank_(0),
        limit_(limit),
        me_(me),
        symbol_(symbol)
    {
    	index_block_offset_ = me.getIndexKeyBlockOffset(symbol);
    }

    void prepareIndex() {}

    Int walkIndex(Int start, Int end)
    {
        for (Int c = start; c > end; c--)
        {
        	IndexKey block_rank = me_.indexb(index_block_offset_, c);

        	if (block_rank >= limit_)
        	{
        		return c;
        	}

        	rank_  += block_rank;
        	limit_ -= block_rank;
        }

        return end;
    }

    IndexKey rank() const
    {
    	return rank_;
    }

    bool is_found() const
    {
    	return found_;
    }
};



template <typename TreeType, Int Bits>
class SelectBWWalker;



template <typename TreeType>
class SelectBWWalker<TreeType, 1>: public SelectBWWalkerBase<TreeType> {

	typedef SelectBWWalkerBase<TreeType> Base;

	typedef typename Base::IndexKey IndexKey;
    typedef typename Base::Value 	Value;

public:
    SelectBWWalker(const TreeType& me, Value symbol, IndexKey limit):
        Base(me, symbol, limit)
    {}

    //FIXME: move offsets[] to constructor
    Int walkValues(Int start, Int end)
    {
    	const Value* buffer = Base::me_.valuesBlock();

    	auto result = Base::symbol_?
    			Select1BW(buffer, start, end, Base::limit_) :
    			Select0BW(buffer, start, end, Base::limit_);

    	Base::rank_   += result.rank();
    	Base::limit_  -= result.rank();

    	Base::found_	= result.is_found() || Base::limit_ == 0;

    	return result.idx();
    }
};




template <typename TreeType, Int Bits>
class SelectBWWalker: public SelectBWWalkerBase<TreeType> {

	typedef SelectBWWalkerBase<TreeType> Base;

	typedef typename Base::IndexKey IndexKey;
    typedef typename Base::Value 	Value;

    Int value_block_offset_;

public:
    SelectBWWalker(const TreeType& me, Value symbol, IndexKey limit):
        Base(me, symbol, limit)
    {
    	value_block_offset_ = me.getValueBlockOffset();
    }


    Int walkValues(Int start, Int end)
    {
    	if (Base::limit_ == 0)
    	{
    		Base::found_ = true;
    		return start;
    	}

    	IndexKey total = 0;

    	for (Int c = start; c > end; c--)
    	{
    		total += Base::me_.testb(value_block_offset_, c - 1, Base::symbol_);

    		if (total == Base::limit_)
    		{
    			Base::rank_ 	+= total;
    			Base::found_ 	= true;

    			return c - 1;
    		}
    	}

    	Base::rank_ 	+= total;
    	Base::limit_  	-= total;
    	Base::found_ 	= Base::limit_ == 0;

    	return end;
    }
};







}


#endif
