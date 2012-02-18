
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_BATCH_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_BATCH_HPP

#include <memoria/vapi/models/logs.hpp>

#include <memoria/prototypes/btree/tools.hpp>
#include <memoria/prototypes/btree/pages/tools.hpp>



namespace memoria    {

using namespace memoria::btree;
using namespace memoria::core;

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

    typedef FixedVector<NodeRoomDescr>											NodeRoomVector;

    typedef Accumulators<Key, Counters, Indexes>								Accumulator;

    struct LeafNodeKeyValuePair
    {
    	Key 	keys[Indexes];
    	Value	value;
    };

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

    	virtual NonLeafNodeKeyValuePair GetKVPair(MyType& ctr, Enum direction, BigInt begin, BigInt count, Int level) 	= 0;
    	virtual LeafNodeKeyValuePair 	GetLeafKVPair(MyType& ctr, Enum direction, BigInt begin) 						= 0;
    	virtual BigInt 					GetTotalKeyCount()																= 0;

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

    void UpdateCounters(NodeBaseG& node, Int idx, const Accumulator& counters) const
    {
    	// FIXME: type/node-specific counters application

    	node->counters() += counters.counters();
    	me()->AddKeys(node, idx, counters.keys());
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
    	return me()->InsertSubtree(iter.page(), iter.key_idx(), provider);
    }

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

    	Accumulator accum;
    	Accumulator accum_left;
    	Accumulator accum_right;

    	BigInt start;
    	BigInt end;
    	BigInt total;

    	BigInt remains;

    	InsertSharedData(ISubtreeProvider& provider_): provider(provider_), start(0), end(0), total(provider_.GetTotalKeyCount()), remains(total) {}
    };


    void InsertSubtreeAtStart(NodeBaseG& node, Int& idx, InsertSharedData& data)
    {

    }

    void InsertSubtreeAtEnd(NodeBaseG& node, Int idx, InsertSharedData& data)
    {

    }

    void InsertSubtreeInTheMiddle(NodeBaseG& node, Int &idx, InsertSharedData& data)
    {
    	NodeBaseG left_node = node;
    	InsertSubtreeInTheMiddle(left_node, idx, node, idx, data);
    }



    void InsertSubtreeInTheMiddle(NodeBaseG& left_node, Int left_idx, NodeBaseG& right_node, Int& right_idx, InsertSharedData& data);


private:

    void ReindexAndUpdateCounters(NodeBaseG& node) const
    {
    	me()->Reindex(node);
    	me()->UpdateParentLinksAndCounters(node);
    }

    void MakeRoom(NodeBaseG& node, Int start, Int count) const
    {
    	me()->InsertSpace(node, start, count);
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
			InsertSubtreeInTheMiddle(node, idx, data);
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

	return data.accum;
}










//// ----------------------  PRIVATE API ------------------------- ////



M_PARAMS
void M_TYPE::InsertSubtreeInTheMiddle(NodeBaseG& left_node, Int left_idx, NodeBaseG& right_node, Int& right_idx, InsertSharedData& data)
{
	//FIXME: Check node->level() after deletion;

	Int 	level 			= left_node->level();
	BigInt 	subtree_size 	= me()->GetSubtreeSize(level);

	BigInt  key_count		  	= data.total - data.start - data.end;

	if (left_node == right_node)
	{
		//FIXME: are operands strictly correct for this call?
		Int max_node_capacity 	= me()->GetMaxKeyCountForNode(left_node->is_root(), level == 0, level);


		if (key_count <= subtree_size * max_node_capacity)
		{
			// We have enough free space for all subtrees in the current node

			BigInt total 	= Divide(key_count, subtree_size);

			MakeRoom(left_node, left_idx, total);

			FillNodeLeft(left_node, left_idx, total, data);

			data.accum_left  += data.accum_right;
			data.accum_right =  data.accum_left;

			right_idx 		 += total;

			// proceed update counters/keys up to the root
			me()->UpdateParentIfExists(left_node, data.accum_left);
		}
		else
		{
			// There is no enough free space in current node for all subtrees
			// Split the node and proceed
			NodeBaseG new_left_node 	= left_node;
			NodeBaseG new_right_node 	= me()->SplitBTreeNode(new_left_node, left_idx, 0);

			right_idx = 0;
			InsertSubtreeInTheMiddle(new_right_node, left_idx, new_left_node, right_idx, data);
		}
	}
	else {
		Int start_capacity 	= me()->GetCapacity(left_node);
		Int end_capacity 	= me()->GetCapacity(right_node);
		Int total_capacity	= start_capacity + end_capacity;


		BigInt max_key_count = subtree_size * total_capacity;

		if (key_count <= max_key_count)
		{
			// If there are more keys than these nodes can store,
			// fill nodes fully

			FillNodeLeft(left_node,   left_idx,  start_capacity,  data);
			FillNodeRight(right_node, right_idx, end_capacity, data);

			right_idx += end_capacity;

			me()->UpdateParentIfExists(left_node,  data.accum_left);
			me()->UpdateParentIfExists(right_node, data.accum_right);
		}
		else
		{
			// Otherwise fill nodes 'equally'. Each node will have almost
			// equal free space after processing.

			BigInt total_keys 			= Divide(key_count, subtree_size);
			BigInt total_keys_in_node 	= total_keys > total_capacity ? total_capacity : total_keys;


			Int free_space 	= total_capacity - total_keys_in_node;
			Int delta		= free_space / 2;

			Int left_count	= start_capacity > delta ? start_capacity - delta : 0;
			Int right_count	= total_keys_in_node - left_count;

			MakeRoom(left_node,  left_idx, left_count);
			MakeRoom(right_node, right_idx, right_count);

			FillNodeLeft(left_node,   left_idx,  left_count,  data);
			FillNodeRight(right_node, right_idx, right_count, data);

			right_idx = right_count;

			NodeBaseG left_node_parent 	= me()->GetParent(left_node, 	Allocator::READ);
			NodeBaseG right_node_parent = me()->GetParent(right_node, 	Allocator::READ);

			// Update counters only for parent nodes

			UpdateCounters(left_node_parent,  left_node->parent_idx(),  data.accum_left);
			UpdateCounters(right_node_parent, right_node->parent_idx(), data.accum_right);

			// There is something more to insert.
			Int parent_right_idx = left_node_parent == right_node_parent ? left_node_parent->parent_idx() + 1: 0;
			InsertSubtreeInTheMiddle
			(
					left_node_parent,
					left_node_parent->parent_idx() + 1,
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

	if (level > 0)
	{
		for (Int c = from; c < from + count; c++)
		{
			BigInt requested_size = data.remains >= subtree_size ? subtree_size : data.remains;
			NonLeafNodeKeyValuePair pair = data.provider.GetKVPair(*me(), ISubtreeProvider::FORWARD, data.start, requested_size, level);

			me()->SetKeys(node, c, pair.keys);
			me()->SetINodeData(node, c, &pair.value);

			data.start  	+= pair.key_count;
			data.remains	-= pair.key_count;
		}
	}
	else {
		for (Int c = from; c < from + count; c++)
		{
			LeafNodeKeyValuePair pair = data.provider.GetLeafKVPair(*me(), ISubtreeProvider::FORWARD, data.start);

			me()->SetKeys(node, c, pair.keys);
			me()->SetLeafData(node, c, pair.value);

			data.start 		+= 1;
			data.remains	-= 1;
		}
	}

	me()->ReindexAndUpdateCounters(node);

	data.accum_left += me()->GetCounters(node, from, count);
}


M_PARAMS
void M_TYPE::FillNodeRight(NodeBaseG& node, Int from, Int count, InsertSharedData& data)
{
	Int level = node->level();
	BigInt subtree_size = me()->GetSubtreeSize(level);

	if (level > 0)
	{
		for (Int c = count - 1; c >= 0; c--)
		{
			BigInt requested_size = data.remains >= subtree_size ? subtree_size : data.remains;
			NonLeafNodeKeyValuePair pair = data.provider.GetKVPair(*me(), ISubtreeProvider::BACKWARD, data.end, requested_size, level - 1);

			me()->SetKeys(node, c, pair.keys);
			me()->SetINodeData(node, c, &pair.value);

			data.end 		+= pair.key_count;
			data.remains	-= pair.key_count;
		}
	}
	else {
		for (Int c = count - 1; c >= 0; c--)
		{
			LeafNodeKeyValuePair pair = data.provider.GetLeafKVPair(*me(), ISubtreeProvider::FORWARD, data.end);

			me()->SetKeys(node, c, pair.keys);
			me()->SetLeafData(node, c, pair.value);

			data.end 		+= 1;
			data.remains	-= 1;
		}
	}

	me()->ReindexAndUpdateCounters(node);

	data.accum_right += me()->GetCounters(node, from, count);
}



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
