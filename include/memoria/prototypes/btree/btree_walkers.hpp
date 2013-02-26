
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTREE_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BTREE_WALKERS_HPP

#include <memoria/core/tools/fixed_vector.hpp>
#include <memoria/core/container/iterator.hpp>

#include <ostream>

namespace memoria       {
namespace btree         {

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
	typename Types,
	template <typename MainWalker, typename Map, template <typename, typename> class Extender> class NodeWalker,
	template <typename, typename> class WalkerExtender,
	typename ExtenderState
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
	BTreeForwardWalker(BigInt limit, Int block_num, const ExtenderState& state):
		Base(limit, block_num), extender_state_(state)
	{}

	template <typename Map>
	NodeWalker<MyType, Map, WalkerExtender> nodeWalker(const Map& map)
	{
		return NodeWalker<MyType, Map, WalkerExtender>(*this, map, Base::limit_, Base::block_num_);
	}

	ExtenderState& extenderState() {
		return extender_state_;
	}

	const ExtenderState& extenderState() const {
		return extender_state_;
	}
};


template <
	typename Types,
	template <typename MainWalker, typename Map, template <typename, typename> class Extender> class NodeWalker,
	template <typename, typename> class WalkerExtender,
	typename ExtenderState
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
	BTreeBackwardWalker(BigInt limit, Int block_num, const ExtenderState& state):
		Base(limit, block_num), extender_state_(state)
	{}

	template <typename Map>
	NodeWalker<MyType, Map, WalkerExtender> nodeWalker(const Map& map)
	{
		return NodeWalker<MyType, Map, WalkerExtender>(*this, map, Base::limit_, Base::block_num_);
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
	template <typename Walker, typename Map> class Extender
>
class NodeForwardWalker: public NodeWalkerBase {

	typedef typename Map::Key Key;
	typedef typename Map::IndexKey IndexKey;

	BigInt 			sum_				= 0;
	BigInt 			limit_;

	const Key* 		keys_;
	const IndexKey* indexes_;

	MainWalker& 	main_walker_;

	Extender<MainWalker, Map> extender_;

public:
	NodeForwardWalker(MainWalker& main_walker, const Map& map, BigInt limit, Int block_num):
		limit_(limit),
		main_walker_(main_walker),
		extender_(main_walker, map)
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
				extender_.processIndexes(sum_, start, c);
				return c;
			}
		}

		extender_.processIndexes(sum_, start, end);
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
				extender_.processKeys(sum_, start, c);
				return c;
			}
		}

		extender_.processKeys(sum_, start, end);

		return end;
	}

	void finish() {
		main_walker_.adjust(sum_);
	}

	void adjustEnd(const Map& map)
	{
		main_walker_.adjust(-keys_[map.size() - 1]);
		extender_.adjust(map.size() - 1);
	}

	BigInt sum() const {
		return sum_;
	}
};


template <
	typename MainWalker,
	typename Map,
	template <typename K1, typename K2> class Comparator,
	template <typename Walker, typename Map> class Extender
>
class NodeBackwardWalker: public NodeWalkerBase {

	typedef typename Map::Key Key;
	typedef typename Map::IndexKey IndexKey;

	BigInt 			sum_				= 0;
	BigInt 			limit_;

	const Key* 		keys_;
	const IndexKey* indexes_;

	MainWalker& 	main_walker_;

	Extender<MainWalker, Map> extender_;

public:
	NodeBackwardWalker(MainWalker& main_walker, const Map& map, BigInt limit, Int block_num):
		limit_(limit),
		main_walker_(main_walker),
		extender_(main_walker, map)
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
				extender_.processIndexes(sum_, start, c);
				return c;
			}
		}

		extender_.processIndexes(sum_, start, end);
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
				extender_.processKeys(sum_, start, c);
				return c;
			}
		}

		extender_.processKeys(sum_, start, end);

		return end;
	}

	void finish() {
		main_walker_.adjust(sum_);
	}

	void adjustStart(const Map& map)
	{
		main_walker_.adjust(-keys_[0]);
		extender_.adjust(0);
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

template <typename MainWalker, typename Map, template <typename, typename> class Extender>
using NodeLTForwardWalker = NodeForwardWalker<MainWalker, Map, BTreeCompareLT, Extender>;

template <typename MainWalker, typename Map, template <typename, typename> class Extender>
using NodeLEForwardWalker = NodeForwardWalker<MainWalker, Map, BTreeCompareLE, Extender>;

template <typename MainWalker, typename Map, template <typename, typename> class Extender>
using NodeLTBackwardWalker = NodeBackwardWalker<MainWalker, Map, BTreeCompareLT, Extender>;

template <typename MainWalker, typename Map, template <typename, typename> class Extender>
using NodeLEBackwardWalker = NodeBackwardWalker<MainWalker, Map, BTreeCompareLE, Extender>;


template <typename Walker, typename Map>
struct EmptyExtender {

	EmptyExtender(const Walker&, const Map&) {}

	void processIndexes(BigInt sum, Int start, Int end) {}
	void processKeys(BigInt sum, Int start, Int end) 	{}

	void processValues(Int start, Int end) 	{}

	void adjust(BigInt adjustment)			{}
};

struct EmptyExtenderState {};

struct ExternalSumExtenderState {
	Int size_;
	BigInt* values_;

	ExternalSumExtenderState(Int size, BigInt* values):
		size_(size),
		values_(values)
	{}

	Int size() const {
		return size_;
	}

	BigInt& value(Int idx) {
		return values_[idx];
	}

	const BigInt& value(Int idx) const {
		return values_[idx];
	}
};

}
}

#endif

