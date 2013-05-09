
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PVLE_TREE_WALKERS_HPP_
#define MEMORIA_CORE_PVLE_TREE_WALKERS_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/traits.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/bitmap_select.hpp>

#include <functional>

namespace memoria {

using namespace std;



template <typename K1, typename K2>
struct PackedCompareLE {
	bool operator()(K1 k1, K2 k2) {
		return k1 <= k2;
	}
};

template <typename K1, typename K2>
struct PackedCompareLT {
	bool operator()(K1 k1, K2 k2) {
		return k1 < k2;
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
//	IndexKey sum0_;

    BigInt limit_;

    const IndexKey* indexes_;

    Int index_size_;

public:
    FindForwardFnBase(const IndexKey* indexes, BigInt limit, Int index_size):
        sum_(0),
        limit_(limit),
        indexes_(indexes),
        index_size_(index_size)
    {}

    void prepareIndex() {}
    void finish() {}

    Int index_size() const {
    	return index_size_;
    }

    Int walkIndex(Int start, Int end, Int size)
    {
        Comparator<BigInt, IndexKey> compare;

//        BigInt l0 = limit_;

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
//        		sum_ += (l0 - limit_);
        		return c;
        	}
        }

//    	sum_ += (l0 - limit_);

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
	typedef typename TreeType::BufferType 	BufferType;


private:

	const TreeType& me_;

	const BufferType* values_;

public:
	GetValueOffsetFnBase(const TreeType& me, Int limit):
		Base(me.indexes(0), limit, me.index_size()),
		me_(me)
	{
		values_  = me.values();
	}

	Int max_size() const {
		return me_.max_size();
	}

	Int size() const {
		return me_.max_size();
	}

	Int walkLastValuesBlock(Int value_block_num)
	{
		Int offset = value_block_num ? me_.offset(value_block_num) : 0;

		Int pos = value_block_num * TreeType::ValuesPerBranch + offset;
		Int end = me_.max_size();

		VLECompareLE<Int, BigInt> compare;
		Codec codec;

		Int max_size = me_.max_size();

		while (pos < end)
		{
			Int value = pos < end;

			if (compare(value, Base::limit_))
			{
				Base::sum_ 	 ++;
				Base::limit_ --;

				Value val;

				pos += codec.decode(values_, val, pos, max_size);

				Base::me().processValue(val);
			}
			else {
				return pos;
			}
		}

		return end;
	}
};


template <typename TreeType, typename MyType>
class SumValuesFnBase {

protected:

	typedef typename TreeType::IndexKey		IndexKey;
	typedef typename TreeType::Value 		Value;

protected:

	const TreeType& me_;

	const Value* values_;
	const IndexKey* indexes_;

	IndexKey sum_ = 0;

public:
	SumValuesFnBase(const TreeType& me, Int index): me_(me)
	{
		values_ 	= me.values();
		indexes_ 	= me.indexes(index);
	}

	void prepareIndex() {}
	void finish() {}

	Int max_size() const {
		return me_.raw_max_size();
	}

	Int size() const {
		return me_.raw_size();
	}

	Int walkIndex(Int start, Int end, Int size)
	{
		for (Int c = start; c < end; c++)
		{
			sum_ += indexes_[c];
		}

//		me_.processIndexes(start, end);
		return end;
	}

	IndexKey sum() const {
		return sum_;
	}

	Int walkValues(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			sum_ += values_[c];
		}

//		me_.processValues(start, end);

		return end;
	}
};



template <typename TreeType>
class GetVLEValuesSumFn: public GetValueOffsetFnBase<TreeType, GetVLEValuesSumFn<TreeType> > {
	typedef GetValueOffsetFnBase<TreeType, GetVLEValuesSumFn<TreeType> > Base;

	typedef typename TreeType::IndexKey 			IndexKey;
	typedef typename TreeType::Value 				Value;

	const IndexKey* indexes_;

	Value value_ = 0;

public:
	GetVLEValuesSumFn(const TreeType& me, Int limit): Base(me, limit)
	{
		indexes_ = me.indexes(1);
	}

	void processIndexes(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			value_ += indexes_[c];
		}
	}

	void processValue(Value value)
	{
		value_ += value;
	}

	Value value() const {
		return value_;
	}
};





template <
	typename TreeType,
	template <typename, typename> class Comparator
>
class FindElementFn: public FindForwardFnBase<
	TreeType,
	FindElementFn<TreeType, Comparator>,
	typename TreeType::IndexKey,
	Comparator
>
{
	typedef typename TreeType::IndexKey 	IndexKey;
	typedef typename TreeType::Codec 		Codec;
	typedef typename TreeType::Value		Value;
	typedef typename TreeType::BufferType 	BufferType;

	typedef FindForwardFnBase<TreeType, FindElementFn<TreeType, Comparator>, IndexKey, Comparator> 	Base;

public:

private:

	const TreeType& 	me_;

	const BufferType* 	values_;
	const IndexKey* 	sizes_;

	Int position_;

	Int size_;
	Int max_size_;

public:
	FindElementFn(const TreeType& me, BigInt limit, Int index_size, Int size, Int max_size):
		Base(me.indexes(1), limit, index_size),
		me_(me),
		position_(0),
		size_(size),
		max_size_(max_size)
	{
		values_  = me.values();
		sizes_   = me.sizes();
	}

	Int max_size() const {
		return max_size_;
	}

//	Int index_size() const {
//		return index_size_;
//	}

	Int size() const {
		return size_;
	}

	Int position() const {
		return position_;
	}

	Int walkFirstValuesBlock(Int start, Int end)
	{
		return walkValues(start, end);
	}

	Int walkLastValuesBlock(Int value_block_num)
	{
		Int offset = value_block_num ? me_.offset(value_block_num) : 0;

		Int pos = value_block_num * TreeType::ValuesPerBranch + offset;
		Int end = max_size_;

		return walkValues(pos, end);
	}

	Int walkValues(Int pos, Int end)
	{
		Comparator<IndexKey, BigInt> compare;
		Codec codec;

		while (pos < end)
		{
			Value value;

			Int len = codec.decode(values_, value, pos, end);

			if (compare(value, Base::limit_))
			{
				Base::sum_ 	 += value;
				Base::limit_ -= value;

				pos += len;

				position_ ++;
			}
			else {
				return pos;
			}
		}

		return end;
	}

	void processIndexes(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			position_ += sizes_[c];
		}
	}
};


template <typename Tree>
class GetFSEValuesSumFn: public SumValuesFnBase<Tree, GetFSEValuesSumFn<Tree> > {

	typedef SumValuesFnBase<Tree, GetFSEValuesSumFn<Tree> > 		Base;

public:
	GetFSEValuesSumFn(const Tree& me, Int index = 0): Base(me, index) {}

	void processIndexes(Int start, Int end) {}
	void processValues(Int start, Int end) {}
};




template <
	typename Tree,
	template <typename, typename> class Comparator,
	typename MyType
>
class FSEFindElementFnBase: public FindForwardFnBase <
	Tree, MyType, typename Tree::IndexKey, Comparator
> {

	typedef typename Tree::IndexKey 	IndexKey;
	typedef typename Tree::Value 		Value;

	typedef FindForwardFnBase<Tree, MyType, IndexKey, Comparator> 	Base;

public:

private:

	const Tree& 		me_;

	const Value* 		values_;

public:
	FSEFindElementFnBase(const Tree& me, BigInt limit, Int index = 0):
		Base(me.indexes(index), limit, me.index_size()),
		me_(me)
	{
		values_  = me_.values();
	}

	Int max_size() const {
		return me_.max_size();
	}

	Int index_size() const {
		return me_.index_size();
	}

	Int size() const {
		return me_.size();
	}

	Int walkFirstValuesBlock(Int start, Int end)
	{
		return walkValues(start, end);
	}

	Int walkLastValuesBlock(Int value_block_num)
	{
		Int pos = value_block_num * Tree::Types::ValuesPerBranch;
		Int end = me_.size();

		return walkValues(pos, end);
	}

	Int walkValues(Int pos, Int end)
	{
		Comparator<Int, Value> compare;

		for (Int c = pos; c < end; c++)
		{
			Value value = values_[c];

			if (compare(value, Base::limit_))
			{
				Base::sum_ 	 += value;
				Base::limit_ -= value;

				Base::me().processValue(value);
			}
			else {
				Base::me().processValues(pos, c);

//				Base::sum_ += (l0 - Base::limit_);

				return c;
			}
		}

//		Base::sum_ += (l0 - Base::limit_);

		Base::me().processValues(pos, end);
		return end;
	}

	void processIndexes(Int start, Int end) {}
	void processValues(Int start, Int end)	{}
	void processValue(Value value)	{}
};


template <
	typename Tree,
	template <typename, typename> class Comparator
>
class FSEFindElementFn: public FSEFindElementFnBase<Tree, Comparator, FSEFindElementFn<Tree, Comparator>> {
	typedef FSEFindElementFnBase<Tree, Comparator, FSEFindElementFn<Tree, Comparator>> Base;
public:
	FSEFindElementFn(const Tree& me, BigInt limit, Int index = 0): Base(me, limit, index) {}
};


template <typename TreeType, template <typename, typename> class BaseClass>
class FSEFindExtender: public BaseClass<TreeType, FSEFindExtender<TreeType, BaseClass>>
{
	typedef BaseClass<TreeType, FSEFindExtender<TreeType, BaseClass>> Base;

public:

	FSEFindExtender(const TreeType& me, BigInt limit, Int index = 0): Base(me, limit, index) {}

	void processIndexes(Int start, Int end)
	{
	}
};



//template <typename TreeType, typename MyType>
//class SumValuesFnBase<TreeType, >


template <typename TreeType>
class BitRankFn: public SumValuesFnBase<TreeType, BitRankFn<TreeType>> {

	typedef SumValuesFnBase<TreeType, BitRankFn<TreeType>> 		Base;

	typedef typename TreeType::Value 							Value;
	typedef typename TreeType::IndexKey 						IndexKey;

	const Value* values_;
	Int bit_;

public:
	BitRankFn(const TreeType& tree, Int bit): Base(tree, bit), values_(tree.values()), bit_(bit)
	{}

	void walkValues(Int start, Int end)
	{
		IndexKey& sum = Base::sum_;

		Int rank1 = PopCount(values_, start, end);

		if (bit_) {
			sum += rank1;
		}
		else {
			sum += (end - start) - rank1;
		}
	}
};


template <typename TreeType>
class BitSelectFn: public FindForwardFnBase<TreeType, BitSelectFn<TreeType>, typename TreeType::IndexKey, PackedCompareLT> {

	typedef FindForwardFnBase<TreeType, BitSelectFn<TreeType>, typename TreeType::IndexKey, PackedCompareLT> 	Base;

	typedef typename TreeType::Value 							Value;
	typedef typename TreeType::IndexKey 						IndexKey;

	const TreeType& tree_;

	const Value* values_;
	Int bit_;

	bool found_;

	Int rank_;

public:
	BitSelectFn(const TreeType& tree, Int rank, Int bit):
		Base(tree.indexes(bit), rank, tree.index_size()),
		tree_(tree),
		values_(tree.values()),
		bit_(bit)
	{}

	Int walkLastValuesBlock(Int start)
	{
		BigInt& limit 	= Base::limit_;

		start *= TreeType::ValuesPerBranch;

		auto result = bit_ ?
				Select1FW(values_, start, tree_.size(), limit) :
				Select0FW(values_, start, tree_.size(), limit);

		rank_  = result.rank();
		found_ = result.is_found();

		return result.idx();
	}

	void processIndexes(Int start, Int c) {}

	bool is_found() const {
		return found_;
	}

	Int rank() const {
		return rank_;
	}

	Int max_size() {
		return tree_.max_size();
	}

	Int size() {
		return tree_.size();
	}
};


}

#endif
