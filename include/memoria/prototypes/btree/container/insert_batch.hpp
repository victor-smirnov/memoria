
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_BATCH_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_BATCH_HPP

#include <memoria/vapi/models/logs.hpp>

#include <memoria/prototypes/btree/tools.hpp>
#include <memoria/prototypes/btree/pages/tools.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::btree;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::InsertBatchName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;
    

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::TreeNodePage                                     	TreeNodePage;
    typedef typename Base::Counters                                             Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Node2RootMap                                         Node2RootMap;
    typedef typename Base::Root2NodeMap                                         Root2NodeMap;

    typedef typename Base::NodeFactory                                          NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    static const Int Indexes                                                    = Base::Indexes;

    typedef typename Base::Metadata                                             Metadata;

//    typedef FixedVector<NodeRoomDescr>											NodeRoomVector;

    typedef Accumulators<Key, Counters, Indexes>								Accumulator;



    struct LeafNodeKeyValuePair
    {
    	Key 	keys[Indexes];
    	Value	value;
    };

    typedef vector<LeafNodeKeyValuePair>										LeafPairsVector;

    struct NonLeafNodeKeyValuePair
    {
    	Key 	keys[Indexes];
    	ID		value;
    	BigInt	key_count;
    };

    struct ISubtreeProvider
    {
    	typedef enum {FORWARD, BACKWARD} 	Enum;
    	typedef MyType 						CtrType;

    	virtual NonLeafNodeKeyValuePair GetKVPair(Enum direction, BigInt begin, BigInt count, Int level) 	= 0;
    	virtual LeafNodeKeyValuePair 	GetLeafKVPair(Enum direction, BigInt begin) 						= 0;
    	virtual BigInt 					GetTotalKeyCount()													= 0;

    	virtual ISubtreeProvider&		GetProvider() {return *this;}
    };

    BigInt Divide(BigInt op1, BigInt op2)
    {
    	return (op1 / op2) + ((op1 % op2 == 0) ?  0 : 1);
    }

    BigInt GetSubtreeSize(Int level)
    {
    	BigInt result = 1;

    	for (int c = 0; c < level; c++)
    	{
    		BigInt children_at_level = me()->GetMaxKeyCountForNode(false, c == 0, c);

    		result *= children_at_level;
    	}

    	return result;
    }




    Accumulator GetCounters(const NodeBaseG& node, Int from, Int count) const
    {
    	Accumulator counters;

    	me()->SumKeys(node, from, count, counters.keys());

    	if (!node->is_leaf())
    	{
    		for (Int c = from; c < count; c++)
    		{
    			NodeBaseG child 	=  me()->GetChild(node, c, Allocator::READ);
    			counters.counters() += child->counters();
    		}
    	}
    	else {
    		counters.counters().key_count() = count;
    	}

    	return counters;
    }



    void UpdateUp(NodeBaseG& node, Int idx, const Accumulator& counters)
    {
    	me()->UpdateCounters(node, idx, counters);

    	if (!node->is_root())
    	{
    		NodeBaseG parent = me()->GetParent(node, Allocator::READ);
    		UpdateUp(parent, node->parent_idx(), counters);
    	}
    }

    void UpdateParentIfExists(NodeBaseG& node, const Accumulator& counters)
    {
    	if (!node->is_root())
    	{
    		NodeBaseG parent = me()->GetParent(node, Allocator::READ);
    		me()->UpdateUp(parent, node->parent_idx(), counters);
    	}
    }

    Accumulator InsertSubtree(NodeBaseG& node, Int &idx, ISubtreeProvider& provider);

    Accumulator InsertSubtree(Iterator& iter, ISubtreeProvider& provider)
    {
    	iter.KeyNum() += provider.GetTotalKeyCount();
    	return me()->InsertSubtree(iter.page(), iter.key_idx(), provider);
    }




    class DefaultSubtreeProviderBase: public ISubtreeProvider {

    	BigInt 	total_;
    	MyType& ctr_;

    public:

    	typedef typename ISubtreeProvider::Enum Direction;

    	DefaultSubtreeProviderBase(MyType& ctr, BigInt total): total_(total), ctr_(ctr) {}

    	virtual NonLeafNodeKeyValuePair GetKVPair(Direction direction, BigInt begin, BigInt total, Int level)
    	{
    		BigInt local_count = 0;
    		return BuildTree(direction, begin, local_count, total, level - 1);
    	}

    	virtual BigInt GetTotalKeyCount() {
    		return total_;
    	}

    	MyType& ctr() {
    		return ctr_;
    	}

    	const MyType& ctr() const {
    		return ctr_;
    	}


    private:
    	NonLeafNodeKeyValuePair BuildTree(Direction direction, BigInt start, BigInt& count, const BigInt total, Int level)
    	{
    		NonLeafNodeKeyValuePair pair;
    		pair.key_count = 0;

    		Int max_keys = ctr_.GetMaxKeyCountForNode(false, level == 0, level);

    		if (level > 0)
    		{
    			NonLeafNodeKeyValuePair children[1000];

    			Int local = 0;
    			for (Int c = 0; c < max_keys && count < total; c++, local++)
    			{
    				children[c] 	=  BuildTree(direction, start, count, total, level - 1);
    				pair.key_count 	+= children[c].key_count;
    			}

    			if (direction == Direction::BACKWARD)
    			{
    				SwapVector(children, local);
    			}

    			NodeBaseG node = ctr_.CreateNode(level, false, false);

    			SetINodeData(children, node, local);
    			ctr_.UpdateParentLinksAndCounters(node);

    			ctr_.GetMaxKeys(node, pair.keys);
    			pair.value = node->id();
    		}
    		else
    		{
    			LeafNodeKeyValuePair children[1000];

    			Int local = 0;
    			for (Int c = 0; c < max_keys && count < total; c++, local++, count++)
    			{
    				children[c] =  this->GetLeafKVPair(direction, start + count);
    			}

    			if (direction == Direction::BACKWARD)
    			{
    				SwapVector(children, local);
    			}

    			NodeBaseG node = ctr_.CreateNode(level, false, true);

    			SetLeafNodeData(children, node, local);
    			ctr_.GetMaxKeys(node, pair.keys);

    			ctr_.UpdateParentLinksAndCounters(node);

    			pair.value 		=  node->id();
    			pair.key_count 	+= local;
    		}

    		return pair;
    	}

    	template <typename PairType, typename ParentPairType>
    	struct SetNodeValuesFn
    	{
    		PairType* 		pairs_;
    		Int 			count_;

    		ParentPairType	total_;

    		SetNodeValuesFn(PairType* pairs, Int count): pairs_(pairs), count_(count) {}

    		template <typename Node>
    		void operator()(Node* node)
    		{
    			for (Int c = 0; c < count_; c++)
    			{
    				for (Int d = 0; d < Indexes; d++)
    				{
    					node->map().key(d, c) = pairs_[c].keys[d];
    				}

    				node->map().data(c) = pairs_[c].value;
    			}

    			node->set_children_count(count_);

    			node->map().Reindex();

    			for (Int d = 0; d < Indexes; d++)
    			{
    				total_.keys[d] = node->map().max_key(d);
    			}

    			total_.value = node->id();
    		}
    	};

    	template <typename PairType>
    	NonLeafNodeKeyValuePair SetINodeData(PairType* data, NodeBaseG& node, Int count)
    	{
    		SetNodeValuesFn<PairType, NonLeafNodeKeyValuePair> fn(data, count);
    		NonLeafDispatcher::Dispatch(node, fn);
    		return fn.total_;
    	}

    	template <typename PairType>
    	NonLeafNodeKeyValuePair SetLeafNodeData(PairType* data, NodeBaseG& node, Int count)
    	{
    		SetNodeValuesFn<PairType, NonLeafNodeKeyValuePair> fn(data, count);
    		LeafDispatcher::Dispatch(node, fn);
    		return fn.total_;
    	}

    	template <typename PairType>
    	void SwapVector(PairType* data, Int total)
    	{
    		for (Int c = 0; c < total; c++)
    		{
    			PairType tmp 			= data[c];
    			data[c] 				= data[total - c  - 1];
    			data[total - c  - 1] 	= tmp;
    		}
    	}
    };


    class ArraySubtreeProvider: public DefaultSubtreeProviderBase
    {
    	typedef DefaultSubtreeProviderBase 		Base;
    	typedef typename ISubtreeProvider::Enum Direction;

    	const LeafNodeKeyValuePair* pairs_;
    public:
    	ArraySubtreeProvider(MyType& ctr, BigInt total, const LeafNodeKeyValuePair* pairs): Base(ctr, total), pairs_(pairs) {}

    	virtual LeafNodeKeyValuePair GetLeafKVPair(Direction direction, BigInt begin)
    	{
    		if (direction == Direction::FORWARD)
    		{
    			return pairs_[begin];
    		}
    		else {
    			return pairs_[Base::GetTotalKeyCount() - begin - 1];
    		}
    	}
    };


    void InsertBatch(Iterator& iter, const LeafNodeKeyValuePair* pairs, BigInt size)
    {
    	ArraySubtreeProvider provider(*me(), size, pairs);

    	me()->InsertSubtree(iter, provider);
    }

    void InsertBatch(Iterator& iter, const LeafPairsVector& pairs)
    {
    	ArraySubtreeProvider provider(*me(), pairs.size(), &pairs.at(0));

    	me()->InsertSubtree(iter, provider);
    }

    void UpdateParentLinksAndCounters(NodeBaseG& node) const
    {
    	node.update();
    	if (node->is_leaf())
    	{
    		node->counters().page_count() 	= 1;
    		node->counters().key_count()	= node->children_count();
    	}
    	else {
    		UpdateParentLinksFn fn(*me(), node);
    		NonLeafDispatcher::Dispatch(node, fn);
    	}
    }

private:

    struct InsertSharedData
    {
    	ISubtreeProvider& provider;

    	Accumulator accumulator;

    	BigInt start;
    	BigInt end;
    	BigInt total;

    	BigInt remains;

    	InsertSharedData(ISubtreeProvider& provider_): provider(provider_), start(0), end(0), total(provider_.GetTotalKeyCount()), remains(total) {}
    };

    //FIXME Implement it
    void InsertSubtreeIntoEmptyCtr(NodeBaseG& node, Int& idx, InsertSharedData& data)
    {
    	//Create empty root+leaf node
    	//InsertSubtreeInTheMiddle(node, idx, data);
    }

    void InsertSubtreeAtStart(NodeBaseG& node, Int& idx, InsertSharedData& data)
    {
    	InsertSubtreeInTheMiddle(node, idx, data);
    }

    void InsertSubtreeAtEnd(NodeBaseG& node, Int& idx, InsertSharedData& data)
    {
    	InsertSubtreeInTheMiddle(node, idx, data);
    }

    void InsertSubtreeInTheMiddle(NodeBaseG& node, Int &idx, InsertSharedData& data)
    {
    	NodeBaseG left_node = node;
    	InsertSubtreeInTheMiddle(left_node, idx, node, idx, data);
    	me()->AddTotalKeyCount(data.provider.GetTotalKeyCount());
    }



    void InsertSubtreeInTheMiddle(NodeBaseG& left_node, Int left_idx, NodeBaseG& right_node, Int& right_idx, InsertSharedData& data);


    struct UpdateParentLinksFn
    {
    	const MyType&			ctr_;
    	NodeBaseG&		node_;

    	UpdateParentLinksFn(const MyType& ctr, NodeBaseG& node):
    		ctr_(ctr),
    		node_(node)
    	{}

    	template <typename Node>
    	void operator()(Node* node)
    	{
    		Counters counters;

    		for (Int c = 0; c < node->children_count(); c++)
    		{
    			ID id 				= node->map().data(c);
    			NodeBaseG child 	= ctr_.allocator().GetPage(id, Allocator::UPDATE);

    			child->parent_id() 	= node_->id();
    			child->parent_idx() = c;

    			counters 			+= child->counters();
    		}

    		node->counters() 				=  counters;
    		node->counters().page_count() 	+= 1;
    	}
    };




    void ReindexAndUpdateCounters(NodeBaseG& node) const
    {
    	me()->Reindex(node);
    	me()->UpdateParentLinksAndCounters(node);
    }

    void MakeRoom(NodeBaseG& node, Int start, Int count) const
    {
    	me()->InsertSpace(node, start, count);
    }

    void UpdateCounters(NodeBaseG& node, Int idx, const Accumulator& counters) const
    {
    	// FIXME: type/node-specific counters application
    	node.update();
    	node->counters() += counters.counters();
    	me()->AddKeys(node, idx, counters.keys());
    }

    void FillNodeLeft(NodeBaseG& node, Int from, Int count, InsertSharedData& data);
    void FillNodeRight(NodeBaseG& node, Int from, Int count, InsertSharedData& data);


MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::InsertBatchName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::InsertSubtree(NodeBaseG& node, Int &idx, ISubtreeProvider& provider)
{
	InsertSharedData data(provider);

	if (idx == 0)
	{
		NodeBaseG prev = Iterator(*me()).GetPrevNode(node);

		if (prev.is_set())
		{
			InsertSubtreeInTheMiddle(prev, prev->children_count(), node, idx, data);
			me()->AddTotalKeyCount(data.provider.GetTotalKeyCount());
		}
		else
		{
			InsertSubtreeAtStart(node, idx, data);
		}
	}
	else if (idx  == node->children_count())
	{
		InsertSubtreeAtEnd(node, idx, data);
	}
	else
	{
		InsertSubtreeInTheMiddle(node, idx, data);
	}

	return data.accumulator;
}




//// ----------------------  PRIVATE API ------------------------- ////



M_PARAMS
void M_TYPE::InsertSubtreeInTheMiddle(NodeBaseG& left_node, Int left_idx, NodeBaseG& right_node, Int& right_idx, InsertSharedData& data)
{
	//FIXME: Check node->level() after deletion;

	Int 	level 			= left_node->level();
	BigInt 	subtree_size 	= me()->GetSubtreeSize(level);

	BigInt  key_count		= data.total - data.start - data.end;

	if (left_node == right_node)
	{
		Int node_capacity = me()->GetCapacity(left_node);

		if (key_count <= subtree_size * node_capacity)
		{
			// We have enough free space for all subtrees in the current node
			BigInt total 	= Divide(key_count, subtree_size);

			FillNodeLeft(left_node, left_idx, total, data);

			right_idx 		+= total;
		}
		else
		{
			// There is no enough free space in current node for all subtrees
			// Split the node and proceed

			right_node = me()->SplitBTreeNode(left_node, left_idx, 0);
			right_idx  = 0;

			InsertSubtreeInTheMiddle(left_node, left_idx, right_node, right_idx, data);
		}
	}
	else {
		Int start_capacity 	= me()->GetCapacity(left_node);
		Int end_capacity 	= me()->GetCapacity(right_node);
		Int total_capacity	= start_capacity + end_capacity;


		BigInt max_key_count = subtree_size * total_capacity;

		if (key_count <= max_key_count)
		{
			// Otherwise fill nodes 'equally'. Each node will have almost
			// equal free space after processing.

			BigInt total_keys 			= Divide(key_count, subtree_size);
			BigInt total_keys_in_node 	= total_keys > total_capacity ? total_capacity : total_keys;

			Int left_count	= start_capacity > total_keys_in_node ? total_keys_in_node : start_capacity;
			Int right_count	= total_keys_in_node - left_count;

			FillNodeLeft(left_node,   left_idx,  left_count,  data);
			FillNodeRight(right_node, right_idx, right_count, data);

			right_idx = right_count;
		}
		else
		{
			// If there are more keys than these nodes can store,
			// fill nodes fully

			FillNodeLeft(left_node,   left_idx,  start_capacity,  data);
			FillNodeRight(right_node, right_idx, end_capacity, data);

			right_idx += end_capacity;

			NodeBaseG left_node_parent 	= me()->GetParent(left_node, 	Allocator::UPDATE);
			NodeBaseG right_node_parent = me()->GetParent(right_node, 	Allocator::UPDATE);

			// There is something more to insert.
			Int parent_right_idx = right_node->parent_idx();
			InsertSubtreeInTheMiddle
			(
					left_node_parent,
					left_node->parent_idx() + 1,
					right_node_parent,
					parent_right_idx,
					data
			);
		}
	}
}






M_PARAMS
void M_TYPE::FillNodeLeft(NodeBaseG& node, Int from, Int count, InsertSharedData& data)
{
	Int level = node->level();
	BigInt subtree_size = me()->GetSubtreeSize(level);

	MakeRoom(node, from, count);

	if (level > 0)
	{
		for (Int c = from; c < from + count; c++)
		{
			BigInt requested_size = data.remains >= subtree_size ? subtree_size : data.remains;
			NonLeafNodeKeyValuePair pair = data.provider.GetKVPair(ISubtreeProvider::FORWARD, data.start, requested_size, level);

			me()->SetKeys(node, c, pair.keys);
			me()->SetINodeData(node, c, &pair.value);

			data.start  	+= pair.key_count;
			data.remains	-= pair.key_count;
		}
	}
	else {
		for (Int c = from; c < from + count; c++)
		{
			LeafNodeKeyValuePair pair = data.provider.GetLeafKVPair(ISubtreeProvider::FORWARD, data.start);

			me()->SetKeys(node, c, pair.keys);
			me()->SetLeafData(node, c, pair.value);

			data.start 		+= 1;
			data.remains	-= 1;
		}
	}

	ReindexAndUpdateCounters(node);

	Accumulator accumulator = me()->GetCounters(node, from, count);

	me()->UpdateParentIfExists(node, accumulator);

	data.accumulator += accumulator;
}


M_PARAMS
void M_TYPE::FillNodeRight(NodeBaseG& node, Int from, Int count, InsertSharedData& data)
{
	Int level = node->level();
	BigInt subtree_size = me()->GetSubtreeSize(level);

	MakeRoom(node, from, count);

	if (level > 0)
	{
		for (Int c = count - 1; c >= 0; c--)
		{
			BigInt requested_size = data.remains >= subtree_size ? subtree_size : data.remains;
			NonLeafNodeKeyValuePair pair = data.provider.GetKVPair(ISubtreeProvider::BACKWARD, data.end, requested_size, level);

			me()->SetKeys(node, c, pair.keys);
			me()->SetINodeData(node, c, &pair.value);

			data.end 		+= pair.key_count;
			data.remains	-= pair.key_count;
		}
	}
	else {
		for (Int c = count - 1; c >= 0; c--)
		{
			LeafNodeKeyValuePair pair = data.provider.GetLeafKVPair(ISubtreeProvider::BACKWARD, data.end);

			me()->SetKeys(node, c, pair.keys);
			me()->SetLeafData(node, c, pair.value);

			data.end 		+= 1;
			data.remains	-= 1;
		}
	}

	ReindexAndUpdateCounters(node);

	Accumulator accumulator = me()->GetCounters(node, from, count);

	me()->UpdateParentIfExists(node, accumulator);

	data.accumulator += accumulator;
}



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
