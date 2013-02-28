
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTREE_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BTREE_WALKERS_HPP

#include <memoria/core/tools/fixed_vector.hpp>
#include <memoria/core/container/iterator.hpp>

#include <ostream>
#include <functional>

namespace memoria       {
namespace btree         {

using namespace std;

template <typename MyType>
class BTreeWalkerBase {
	Int start_ = 0;
	Int idx_   = 0;

	WalkDirection direction_ = WalkDirection::UP;

public:

	template <typename Node>
	void operator()(const Node* node)
	{
		idx_ = me().dispatchNode(*node, start_);
	}

	MyType& me() {
		return *T2T<MyType*>(this);
	}

	const MyType& me() const {
		return *T2T<const MyType*>(this);
	}

	Int& start() {
		return start_;
	}

	const Int& start() const {
		return start_;
	}

	Int& idx() {
		return idx_;
	}

	const Int& idx() const {
		return idx_;
	}

	WalkDirection& direction() {
		return direction_;
	}

	const WalkDirection& direction() const {
		return direction_;
	}
};


template <
	typename MyType
>
class BTreeForwardWalkerBase: public BTreeWalkerBase<MyType> {

	typedef BTreeWalkerBase<MyType> 											Base;

public:
	template <typename Node>
	Int dispatchNode(const Node& node, Int start)
	{
		typedef typename Node::Map Map;

		const Map& map = node.map();

		auto node_walker = me().nodeWalker(map);

		Int idx = map.findFw(me().start(), node_walker);

		if (idx == map.size() && Base::direction() == WalkDirection::DOWN)
		{
			node_walker.adjustEnd(map);
			idx--;
		}

		return idx;
	}

	MyType& me()
	{
		return *T2T<MyType*>(this);
	}

	const MyType& me() const
	{
		return *T2T<const MyType*>(this);
	}
};







template <
	typename MyType
>
class BTreeBackwardWalkerBase: public BTreeWalkerBase<MyType> {

	typedef BTreeWalkerBase<MyType> 											Base;

public:
	template <typename Node>
	Int dispatchNode(const Node& node, Int start)
	{
		typedef typename Node::Map Map;

		const Map& map = node.map();

		auto node_walker = me().nodeWalker(map);

		Int idx = map.findBw(me().start(), node_walker);

		if (idx == -1 && me().direction() == WalkDirection::DOWN)
		{
			node_walker.adjustStart(map);
			idx++;
		}

		return idx;
	}

	MyType& me()
	{
		return *T2T<MyType*>(this);
	}

	const MyType& me() const
	{
		return *T2T<const MyType*>(this);
	}
};



template <
	typename MyType
>
class DefaultBTreeForwardWalkerBase: public BTreeForwardWalkerBase<MyType> {
protected:

	BigInt sum_			= 0;
	BigInt limit_;

	Int block_num_;

public:

	DefaultBTreeForwardWalkerBase(BigInt limit, Int block_num):
		limit_(limit),
		block_num_(block_num)
	{}

	void adjust(BigInt adjustment)
	{
		sum_ 	+= adjustment;
		limit_ 	-= adjustment;
	}


	BigInt sum() const {
		return sum_;
	}
};


template <
	typename MyType
>
class DefaultBTreeBackwardWalkerBase: public BTreeBackwardWalkerBase<MyType> {
protected:

	BigInt sum_			= 0;
	BigInt limit_;

	Int block_num_;

public:

	DefaultBTreeBackwardWalkerBase(BigInt limit, Int block_num):
		limit_(limit),
		block_num_(block_num)
	{}

	void adjust(BigInt adjustment)
	{
		sum_ 	+= adjustment;
		limit_ 	-= adjustment;
	}


	BigInt sum() const {
		return sum_;
	}
};




template <
	typename 										Types,
	template <
		typename 						MainWalker,
		typename 						Map,
		template <
			typename,
			typename,
			typename
		> 								class Extender,
		typename ExtenderState
	> 												class NodeWalker,
	template <typename, typename, typename> 		class WalkerExtender,
	typename 										ExtenderState
>
class BTreeForwardWalker: public DefaultBTreeForwardWalkerBase<
	BTreeForwardWalker<Types, NodeWalker, WalkerExtender, ExtenderState>
> {

	typedef DefaultBTreeForwardWalkerBase<
				BTreeForwardWalker<Types, NodeWalker, WalkerExtender, ExtenderState>
	>																								Base;

	typedef BTreeForwardWalker<Types, NodeWalker, WalkerExtender, ExtenderState>					MyType;

	ExtenderState extender_state_;

public:
	BTreeForwardWalker(BigInt limit, Int block_num, ExtenderState& state):
		Base(limit, block_num), extender_state_(state)
	{}

	template <typename Map>
	NodeWalker<MyType, Map, WalkerExtender, ExtenderState> nodeWalker(const Map& map)
	{
		return NodeWalker<
				MyType,
				Map,
				WalkerExtender,
				ExtenderState
			   >(*this, map, Base::limit_, Base::block_num_, extender_state_);
	}

	ExtenderState& extenderState() {
		return extender_state_;
	}

	const ExtenderState& extenderState() const {
		return extender_state_;
	}
};


template <
	typename 										Types,

	template <
		typename 						MainWalker,
		typename 						Map,
		template <
			typename,
			typename,
			typename
		> 								class Extender,
		typename 						ExtenderState
	> 												class NodeWalker,

	template <typename, typename, typename>			class WalkerExtender,

	typename 										ExtenderState
>
class BTreeBackwardWalker: public DefaultBTreeBackwardWalkerBase<
	BTreeBackwardWalker<Types, NodeWalker, WalkerExtender, ExtenderState>
> {

	typedef DefaultBTreeBackwardWalkerBase<
				BTreeBackwardWalker<Types, NodeWalker, WalkerExtender, ExtenderState>
	>																								Base;

	typedef BTreeBackwardWalker<Types, NodeWalker, WalkerExtender, ExtenderState>					MyType;

	ExtenderState extender_state_;

public:
	BTreeBackwardWalker(BigInt limit, Int block_num, ExtenderState& state):
		Base(limit, block_num), extender_state_(state)
	{}

	template <typename Map>
	NodeWalker<MyType, Map, WalkerExtender, ExtenderState> nodeWalker(const Map& map)
	{
		return NodeWalker<
				MyType,
				Map,
				WalkerExtender,
				ExtenderState
			   >(*this, map, Base::limit_, Base::block_num_, extender_state_);
	}

	ExtenderState& extenderState() {
		return extender_state_;
	}

	const ExtenderState& extenderState() const {
		return extender_state_;
	}
};



class NodeWalkerBase {
public:
	void prepareIndex() {}
};



template <
	typename MainWalker,
	typename Map,
	template <typename K1, typename K2> class Comparator,
	template <typename Walker, typename MapType, typename State> class Extender,
	typename ExtenderState
>
class NodeForwardWalker: public NodeWalkerBase {

	typedef typename Map::Key Key;
	typedef typename Map::IndexKey IndexKey;

	BigInt 			sum_				= 0;
	BigInt 			limit_;

	const Key* 		keys_;
	const IndexKey* indexes_;

	MainWalker& 	main_walker_;

	Extender<MainWalker, Map, ExtenderState> extender_;

	Int 			block_num_;
public:
	NodeForwardWalker(MainWalker& main_walker, const Map& map, BigInt limit, Int block_num, ExtenderState& state):
		limit_(limit),
		main_walker_(main_walker),
		extender_(main_walker, map, state),
		block_num_(block_num)
	{
		keys_ 		= map.keys(block_num);
		indexes_ 	= map.indexes(block_num);
	}

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
				extender_.processIndexes(sum_, block_num_, start, c);
				return c;
			}
		}

		extender_.processIndexes(sum_, block_num_, start, end);
		return end;
	}

	Int walkKeys(Int start, Int end)
	{
		Comparator<BigInt, Key> compare;

		for (Int c = start; c < end; c++)
		{
			Key key = keys_[c];

			if (compare(key, limit_))
			{
				sum_ 	+= key;
				limit_ 	-= key;
			}
			else {
				extender_.processKeys(sum_, block_num_, start, c);
				return c;
			}
		}

		extender_.processKeys(sum_, block_num_, start, end);

		return end;
	}

	void finish() {
		main_walker_.adjust(sum_);
	}

	void adjustEnd(const Map& map)
	{
		main_walker_.adjust(-keys_[map.size() - 1]);
		extender_.subtract(map.size() - 1);
	}

	BigInt sum() const
	{
		return sum_;
	}
};


template <
	typename MainWalker,
	typename Map,
	template <typename K1, typename K2> class Comparator,
	template <typename Walker, typename MapType, typename State> class Extender,
	typename ExtenderState
>
class NodeBackwardWalker: public NodeWalkerBase {

	typedef typename Map::Key Key;
	typedef typename Map::IndexKey IndexKey;

	BigInt 			sum_				= 0;
	BigInt 			limit_;

	const Key* 		keys_;
	const IndexKey* indexes_;

	MainWalker& 	main_walker_;

	Extender<MainWalker, Map, ExtenderState> extender_;

	Int 			block_num_;

public:
	NodeBackwardWalker(MainWalker& main_walker, const Map& map, BigInt limit, Int block_num, ExtenderState& state):
		limit_(limit),
		main_walker_(main_walker),
		extender_(main_walker, map, state),
		block_num_(block_num)
	{
		keys_ 		= map.keys(block_num);
		indexes_ 	= map.indexes(block_num);
	}

	Int walkIndex(Int start, Int end)
	{
		Comparator<BigInt, IndexKey> compare;

		for (Int c = start; c > end; c--)
		{
			IndexKey key = indexes_[c];

			if (compare(key, limit_))
			{
				sum_ 	+= key;
				limit_ 	-= key;
			}
			else {
				extender_.processIndexes(sum_, block_num_, c + 1, start + 1);
				return c;
			}
		}

		extender_.processIndexes(sum_, block_num_, end + 1, start + 1);
		return end;
	}

	Int walkKeys(Int start, Int end)
	{
		Comparator<BigInt, Key> compare;

		for (Int c = start; c > end; c--)
		{
			Key key = keys_[c];

			if (compare(key, limit_))
			{
				sum_ 	+= key;
				limit_ 	-= key;
			}
			else {
				extender_.processKeys(sum_, block_num_, c + 1,  start + 1);
				return c;
			}
		}

		extender_.processKeys(sum_, block_num_, end + 1, start + 1);

		return end;
	}

	void finish() {
		main_walker_.adjust(sum_);
	}

	void adjustStart(const Map& map)
	{
		main_walker_.adjust(-keys_[0]);
		extender_.subtract(0);
	}

	BigInt sum() const {
		return sum_;
	}
};




template <typename K1, typename K2>
struct BTreeCompareLE {
	bool operator()(K1 k1, K2 k2) {
		return k1 <= k2;
	}
};

template <typename K1, typename K2>
struct BTreeCompareLT {
	bool operator()(K1 k1, K2 k2) {
		return k1 < k2;
	}
};

template <typename MainWalker, typename Map, template <typename, typename, typename> class Extender, typename ExtenderState>
using NodeLTForwardWalker = NodeForwardWalker<MainWalker, Map, BTreeCompareLT, Extender, ExtenderState>;

template <typename MainWalker, typename Map, template <typename, typename, typename> class Extender, typename ExtenderState>
using NodeLEForwardWalker = NodeForwardWalker<MainWalker, Map, BTreeCompareLE, Extender, ExtenderState>;

template <typename MainWalker, typename Map, template <typename, typename, typename> class Extender, typename ExtenderState>
using NodeLTBackwardWalker = NodeBackwardWalker<MainWalker, Map, BTreeCompareLT, Extender, ExtenderState>;

template <typename MainWalker, typename Map, template <typename, typename, typename> class Extender, typename ExtenderState>
using NodeLEBackwardWalker = NodeBackwardWalker<MainWalker, Map, BTreeCompareLE, Extender, ExtenderState>;


template <typename Walker, typename Map, typename State>
struct EmptyExtender {

	EmptyExtender(Walker&, const Map&, State&) {}

	void processIndexes(BigInt sum, Int key_num, Int start, Int end) {}
	void processKeys(BigInt sum, Int key_num, Int start, Int end) 	 {}

	void processValues(BigInt, Int, Int start, Int end) 			 {}
	void subtract(Int idx)											 {}
};



struct EmptyExtenderState {};

template <typename T = BigInt>
class FunctorExtenderState {
protected:
	typedef function<void (T, Int)> ValueFunction;

	Int	 		indexes_;
	const Int* 	idx_numbers_;

	ValueFunction value_;

public:
	FunctorExtenderState(Int indexes, const Int* numbers, ValueFunction value_fn):
		indexes_(indexes), idx_numbers_(numbers), value_(value_fn)
	{}

	Int indexes() const
	{
		return indexes_;
	}

	Int idx(int num) const
	{
		return idx_numbers_[num];
	}

	ValueFunction value() const {
		return value_;
	}
};







template <typename Map, typename State>
class SumExtenderBase {
protected:
	typedef typename Map::IndexKey 	IndexKey;

	static const Int MAX_INDEXES = 8;

	State& state_;

	const IndexKey* indexes_[MAX_INDEXES];

public:
	SumExtenderBase(const Map& map, State& state):
		state_(state)
	{
		if (state.indexes() < 8)
		{
			for (Int c = 0; c < state.indexes(); c++)
			{
				indexes_[c] = map.indexes(state_.idx(c));
			}
		}
		else {
			throw Exception(MA_SRC, SBuf()<<"Requested number of resqested in Extender State inxeses is too large: "
										  <<state.indexes()<<". The limit is 8.");
		}
	}

	void processIndexes(BigInt sum, Int key_num, Int start, Int end)
	{
		process(sum, key_num, start, end, indexes_);
	}

protected:
	template <typename T>
	void process(BigInt sum, Int key_num, Int start, Int end, const T** array)
	{
		for (Int c = 0; c < state_.indexes(); c++)
		{
			if (state_.idx(c) != key_num)
			{
				BigInt local_sum = 0;
				const T* values = array[c];

				for (Int idx = start; idx < end; idx++)
				{
					local_sum += values[idx];
				}

				state_.value()(local_sum, state_.idx(c));
			}
			else {
				state_.value()(sum, state_.idx(c));
			}
		}
	}

	template <typename T>
	void subtractFromState(Int idx, const T** array)
	{
		for (Int c = 0; c < state_.indexes(); c++)
		{
			state_.value()(-array[c][idx], state_.idx(c));
		}
	}
};



template <typename Walker, typename Map, typename State>
class NodeSumExtender: public SumExtenderBase<Map, State> {

	typedef SumExtenderBase<Map, State> Base;

	typedef typename Map::Key 		Key;
	typedef typename Map::IndexKey 	IndexKey;

	const Key* keys_[Base::MAX_INDEXES];

public:

	NodeSumExtender(Walker&, const Map& map, State& state):
		Base(map, state)
	{
		for (Int c = 0; c < state.indexes(); c++)
		{
			keys_[c] = map.keys(Base::state_.idx(c));
		}
	}

	void processKeys(BigInt sum, Int key_num, Int start, Int end)
	{
		this->process(sum, key_num, start, end, keys_);
	}

	void subtract(Int idx)
	{
		this->subtractFromState(idx, keys_);
	}
};

}
}

#endif

