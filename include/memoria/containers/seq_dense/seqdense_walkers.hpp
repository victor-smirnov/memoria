
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_WALKERS_HPP

#include <memoria/prototypes/sequence/sequence_walkers.hpp>

namespace memoria {

using namespace sequence;

template <
	typename Types,
	template <typename, typename> class Comparator
>
class SequenceFWWalker: public MultiPrefixWalkerBase<Types> {
protected:
	typedef MultiPrefixWalkerBase<Types>											Base;
	typedef typename Types::Key 												Key;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;


public:
	SequenceFWWalker(Container& ctr, Key key, Int key_count, const Int* key_nums, Key* prefixes):
		Base(ctr, key, key_count, key_nums, prefixes)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		typedef typename Node::Map Map;

		const Map& map = node->map();

		FindSumPositionFwCompoundFn<Map, Comparator> fn(
				map, Base::key_count_, Base::key_nums_, Base::prefixes_, Base::key_ - Base::prefixes_[0]);

		if (Base::start_ == 0)
		{
			Base::idx_ = map.find(fn);
		}
		else
		{
			Base::idx_ = map.findFw(Base::start_, fn);
		}

		if (Base::idx_ == map.size() && Base::direction_ == WalkDirection::DOWN)
		{
			for (Int c = 0; c < Base::key_count_; c++)
			{
				Base::prefixes_[c] -= map.key(Base::key_nums_[c], map.size() - 1);
			}

			Base::idx_--;
		}
	}
};


template <
	typename Types,
	template <typename, typename> class Comparator
>
class SequenceSimpleWalker: public SequenceFWWalker<Types, Comparator> {
protected:
	typedef SequenceFWWalker<Types, Comparator>									Base;
	typedef typename Types::Key 												Key;
	typedef typename Base::Container 											Container;
	typedef typename Types::DataPageG 											DataPageG;
	typedef typename Types::NodeBaseG 											NodeBaseG;
	typedef typename Types::Allocator 											Allocator;

	DataPageG data_;

	bool empty_ = false;

public:
	SequenceSimpleWalker(Container& ctr, Key key, Int key_count, const Int* key_nums, Key* prefixes):
		Base(ctr, key, key_count, key_nums, prefixes)
	{}

	void finish(const NodeBaseG& node, Int idx)
	{
		if (idx < node->children_count())
		{
			data_ = Base::ctr_.getValuePage(node, idx, Allocator::READ);
		}
	}

	void empty() {
		empty_ = true;
	}

	DataPageG& data() {
		return data_;
	}

	bool is_empty() const {
		return empty_;
	}
};






template <
	typename Types,
	template <typename, typename> class Comparator
>
class SequenceBWWalker: public MultiPrefixWalkerBase<Types> {
protected:
	typedef MultiPrefixWalkerBase<Types>											Base;
	typedef typename Types::Key 												Key;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;


public:
	SequenceBWWalker(Container& ctr, Key key, Int key_count, const Int* key_nums, Key* prefixes):
		Base(ctr, key, key_count, key_nums, prefixes)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		typedef typename Node::Map Map;

		const Map& map = node->map();

		FindSumPositionBwCompoundFn<Map, Comparator> fn(
				map, Base::key_count_, Base::key_nums_, Base::prefixes_, Base::key_ - Base::prefixes_[0]);

		Base::idx_ = map.findBw(Base::start_, fn);

		if (Base::idx_ == -1 && Base::direction_ == WalkDirection::DOWN)
		{
			for (Int c = 0; c < Base::key_count_; c++)
			{
				Base::prefixes_[c] -= map.key(Base::key_nums_[c], 0);
			}

			Base::idx_++;
		}
	}
};








template <typename TreeType, Int size_block = 0>
class BSTreeCountFWWalker {

	typedef typename TreeType::Key 			Key;
	typedef typename TreeType::IndexKey 	IndexKey;

	Int 			block_num_;
	IndexKey 		rank_;

	const Key* 		keys_;
	const IndexKey* indexes_;

	const Key* 		size_keys_;
	const IndexKey* size_indexes_;



public:
	BSTreeCountFWWalker(const TreeType& me, Int block_num): block_num_(block_num), rank_(0)
	{
		keys_ 			= me.keys(block_num);
		indexes_ 		= me.indexes(block_num);

		size_keys_ 		= me.keys(size_block);
		size_indexes_ 	= me.indexes(size_block);
	}

	void prepareIndex() {}
	void finish() {}

	Int walkIndex(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			IndexKey block_rank = indexes_[c];
			IndexKey size 		= size_indexes_[c];

			if (block_rank == size)
			{
				rank_  += block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}


	Int walkKeys(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			IndexKey block_rank = keys_[c];
			IndexKey size 		= size_keys_[c];

			if (block_rank == size)
			{
				rank_  += block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}

	IndexKey rank() const {
		return rank_;
	}
};




template <typename TreeType, Int size_block = 0>
class BSTreeCountBWWalker {

	typedef typename TreeType::Key 			Key;
	typedef typename TreeType::IndexKey 	IndexKey;

	Int 			block_num_;
	IndexKey 		rank_;

	const Key* 		keys_;
	const IndexKey* indexes_;

	const Key* 		size_keys_;
	const IndexKey* size_indexes_;

public:
	BSTreeCountBWWalker(const TreeType& me, Int block_num): block_num_(block_num), rank_(0)
	{
		keys_ 			= me.keys(block_num);
		indexes_ 		= me.indexes(block_num);

		size_keys_ 		= me.keys(size_block);
		size_indexes_ 	= me.indexes(size_block);
	}

	void prepareIndex() {}
	void finish() {}

	Int walkIndex(Int start, Int end)
	{
		for (Int c = start; c > end; c--)
		{
			IndexKey block_rank = indexes_[c];
			IndexKey size 		= size_indexes_[c];

			if (block_rank == size)
			{
				rank_  += block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}


	Int walkKeys(Int start, Int end)
	{
		for (Int c = start; c > end; c--)
		{
			IndexKey block_rank = keys_[c];
			IndexKey size 		= size_keys_[c];

			if (block_rank == size)
			{
				rank_  += block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}

	IndexKey rank() const {
		return rank_;
	}
};









template <
	typename Types
>
class SequenceCountFWWalker: public SinglePrefixWalkerBase<Types> {
protected:
	typedef SinglePrefixWalkerBase<Types>										Base;
	typedef typename Types::Key 												Key;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;


public:
	SequenceCountFWWalker(Container& ctr, Int key_num):
		Base(ctr, key_num)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		typedef typename Node::Map Map;

		const Map& map = node->map();

		BSTreeCountFWWalker<typename Node::Map> walker(map, Base::key_num_);

		Base::idx_ = map.findFw(Base::start_, walker);

		Base::prefix_ += walker.rank();

		if (Base::idx_ == map.size() && Base::direction_ == WalkDirection::DOWN)
		{
			Base::prefix_ -= map.key(Base::key_num_, map.size() - 1);
			Base::idx_--;
		}

	}
};


template <
	typename Types
>
class SequenceCountBWWalker: public SinglePrefixWalkerBase<Types> {
protected:
	typedef SinglePrefixWalkerBase<Types>										Base;
	typedef typename Types::Key 												Key;
	typedef typename Base::Iterator 											Iterator;
	typedef typename Base::Container 											Container;


public:
	SequenceCountBWWalker(Container& ctr, Int key_num):
		Base(ctr, key_num)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		typedef typename Node::Map Map;

		const Map& map = node->map();

		BSTreeCountBWWalker<typename Node::Map> walker(map, Base::key_num_);

		Base::idx_ = map.findBw(Base::start_, walker);

		Base::prefix_ += walker.rank();

		if (Base::idx_ == -1 && Base::direction_ == WalkDirection::DOWN)
		{
			Base::prefix_ -= map.key(Base::key_num_, 0);
			Base::idx_++;
		}
	}
};


namespace sequence {

template <typename Walker, typename Sequence, typename State>
class RankExtender: public SumExtenderBase<Sequence, State> {

	typedef SumExtenderBase<Sequence, State> Base;

	typedef typename Sequence::Symbol 	Symbol;
	typedef typename Sequence::IndexKey 	IndexKey;

	const Sequence& seq_;

public:

	RankExtender(Walker&, const Sequence& seq, State& state):
		Base(seq, state),
		seq_(seq)
	{}

	void processValues(BigInt sum, Int key_num, Int start, Int end)
	{
		for (Int c = 0; c < this->state_.indexes(); c++)
		{
			Int 	idx = this->state_.idx(c);

			size_t 	cnt = seq_.rank1(start, end, idx);

			this->state_.value()(cnt, idx);
		}
	}
};


template <typename Walker, typename Sequence, typename State>
class SelectExtender: public SumExtenderBase<Sequence, State> {

	typedef SumExtenderBase<Sequence, State> Base;

	typedef typename Sequence::Symbol 	Symbol;
	typedef typename Sequence::IndexKey 	IndexKey;

	const Sequence& seq_;

public:

	SelectExtender(Walker&, const Sequence& seq, State& state):
		Base(seq, state),
		seq_(seq)
	{}

	void processValues(BigInt sum, Int key_num, Int start, Int end)
	{
		for (Int c = 0; c < this->state_.indexes(); c++)
		{
			Int 	idx = this->state_.idx(c);

			size_t 	cnt = seq_.rank1(start, end, idx);

			this->state_.value()(cnt, idx);
		}
	}
};


template <
	typename Sequence,
	typename MainWalker,
	template <typename Walker, typename Map, typename State> class Extender,
	typename State
>
class SelectForwardWalker :public DataForwardWalkerBase<Sequence, MainWalker, BTreeCompareLT, Extender, State> {

	typedef DataForwardWalkerBase<Sequence, MainWalker, BTreeCompareLT, Extender, State> 		Base;
	typedef SelectForwardWalker<Sequence, MainWalker, Extender, State> 							MyType;

	typedef typename Sequence::Symbol											Symbol;

	const Symbol* symbols_;

	bool found_ = false;

	const Sequence& seq_;

public:

	SelectForwardWalker(MainWalker& main_walker, const Sequence& data, BigInt limit, Int block_num, State& state):
		Base(main_walker, data, limit, block_num, state),
		seq_(data)
	{
		symbols_ 	= data.valuesBlock();
	}

	Int walkValues(Int start, Int end)
	{
		auto& sum_ 			= Base::sum_;
		auto& limit_		= Base::limit_;
		auto& block_num_	= Base::block_num_;

		Int idx;

		if (Sequence::Bits == 1)
		{
			auto result = block_num_?
					Select1FW(symbols_, start, end, limit_) :
					Select0FW(symbols_, start, end, limit_);

			sum_    += result.rank();
			limit_  -= result.rank();

			idx = result.idx();

			found_  = result.is_found() || limit_ == 0;
		}
		else {
			Int total = 0;

			found_ = false;

			idx = end;

			Int value_block_offset = seq_.getValueBlockOffset();

			for (Int c = start; c < end; c++)
			{
				total += seq_.testb(value_block_offset, c, Base::block_num_);

				if (total == limit_)
				{
					idx = c;
					break;
				}
			}

			sum_ 	+= total;
			limit_  -= total;

			found_ 	= limit_ == 0;
		}

		if (found_)
		{
			Base::extender_.processValues(sum_, block_num_, start, idx + 1);
		}
		else {
			Base::extender_.processValues(sum_, block_num_, start, end);
		}

		return idx;
	}

	Int walkIndex(Int start, Int end, Int)
	{
		return Base::walkIndex(start, end);
	}

	bool is_found() const
	{
		return found_;
	}
};





template <
	typename Sequence,
	typename MainWalker,
	template <typename Walker, typename Map, typename State> class Extender,
	typename State
>
class SelectBackwardWalker :public DataBackwardWalkerBase<Sequence, MainWalker, BTreeCompareLT, Extender, State> {

	typedef DataBackwardWalkerBase<Sequence, MainWalker, BTreeCompareLT, Extender, State> 		Base;
	typedef SelectBackwardWalker<Sequence, MainWalker, Extender, State> 						MyType;

	typedef typename Sequence::Symbol											Symbol;

	const Symbol* symbols_;

	bool found_ = false;

	const Sequence& seq_;

public:

	SelectBackwardWalker(MainWalker& main_walker, const Sequence& data, BigInt limit, Int block_num, State& state):
		Base(main_walker, data, limit, block_num, state),
		seq_(data)
	{
		symbols_ 	= data.valuesBlock();
	}

	Int walkValues(Int start, Int end)
	{
		auto& sum_ 			= Base::sum_;
		auto& limit_		= Base::limit_;
		auto& block_num_	= Base::block_num_;

		Int idx;

		if (Sequence::Bits == 1)
		{
			auto result = block_num_?
					Select1BW(symbols_, start, end, limit_) :
					Select0BW(symbols_, start, end, limit_);

			sum_    += result.rank();
			limit_  -= result.rank();

			idx = result.idx();

			found_  = result.is_found() || limit_ == 0;
		}
		else {
	    	if (limit_ == 0)
	    	{
	    		found_ = true;
	    		return start;
	    	}

	    	found_ = false;

	    	Int total 				= 0;
	    	Int idx					= end;
	    	Int value_block_offset 	= seq_.getValueBlockOffset();

	    	for (Int c = start; c > end; c--)
	    	{
	    		total += seq_.testb(value_block_offset, c - 1, Base::block_num_);

	    		if (total == Base::limit_)
	    		{
	    			idx = c - 1;
	    			break;
	    		}
	    	}

	    	sum_ 	+= total;
	    	limit_  -= total;
	    	found_ 	= limit_ == 0;

	    	return idx;
		}

		if (found_)
		{
			Base::extender_.processValues(sum_, block_num_, idx + 1, start + 1);
		}
		else {
			Base::extender_.processValues(sum_, block_num_, end + 1, start + 1);
		}

		return idx;
	}

	Int walkIndex(Int start, Int end, Int)
	{
		return Base::walkIndex(start, end);
	}

	bool is_found() const
	{
		return found_;
	}
};



}



}

#endif
