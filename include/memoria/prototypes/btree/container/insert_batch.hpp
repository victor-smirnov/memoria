
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

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::TreeNodePage                                     	TreeNodePage;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Accumulator											Accumulator;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int Indexes                                                    = Base::Indexes;

    struct LeafNodeKeyValuePair
    {
    	Accumulator keys;
    	Value		value;
    };

    typedef vector<LeafNodeKeyValuePair>										LeafPairsVector;

    struct NonLeafNodeKeyValuePair
    {
    	Accumulator keys;
    	ID			value;
    	BigInt		key_count;
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

    BigInt GetSubtreeSize(Int level) const;


    Accumulator GetCounters(const NodeBaseG& node, Int from, Int count) const;
    void MakeRoom(TreePath& path, Int level, Int start, Int count) const;
    void UpdateUp(TreePath& path, Int level, Int idx, const Accumulator& counters);
    void UpdateParentIfExists(TreePath& path, Int level, const Accumulator& counters);
    Accumulator InsertSubtree(Iterator& iter, ISubtreeProvider& provider);

    void InsertEntry(Iterator& iter, const Element&);

    bool Insert(Iterator& iter, const Element& element)
    {
    	InsertEntry(iter, element);
    	return iter.NextKey();
    }

    void InsertBatch(Iterator& iter, const LeafNodeKeyValuePair* pairs, BigInt size);
    void InsertBatch(Iterator& iter, const LeafPairsVector& pairs);


//    TreePathItem SplitPath(TreePath& path, Int level, Int idx);
    void 		 SplitPath(TreePath& left, TreePath& right, Int level, Int idx);
    void		 NewRoot(TreePath& path);


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


    			pair.keys = ctr_.GetMaxKeys(node);
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
    			pair.keys = ctr_.GetMaxKeys(node);

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


    void InsertSubtree(TreePath& path, Int &idx, InsertSharedData& data)
    {
    	TreePath left_path = path;
    	InsertSubtree(left_path, idx, path, idx, 0, data);

    	me()->FinishPathStep(path, idx);

    	me()->AddTotalKeyCount(data.provider.GetTotalKeyCount());
    }


    void InsertSubtree(TreePath& left_path, Int left_idx, TreePath& right_path, Int& right_idx, Int level, InsertSharedData& data);



    void ReindexAndUpdateCounters(NodeBaseG& node) const
    {
    	me()->Reindex(node);
    }



    Accumulator MoveElements(NodeBaseG& srt, NodeBaseG& tgt, Int from, Int tgt_shift = 0) const;

    bool UpdateCounters(NodeBaseG& node, Int idx, const Accumulator& counters) const;

    void FillNodeLeft(TreePath& path, Int level, Int from, Int count, InsertSharedData& data);
    void FillNodeRight(TreePath& path, Int level, Int from, Int count, InsertSharedData& data);


    TreePathItem Split(TreePath& path, Int level, Int idx);
    void		 Split(TreePath& left, TreePath& right, Int level, Int idx);


    class MakeRoomFn {
        Int from_, count_;

    public:
        MakeRoomFn(Int from, Int count):
        	from_(from), count_(count)
        {}

        template <typename Node>
        void operator()(Node *node)
        {
            node->map().InsertSpace(from_, count_);

            for (Int c = from_; c < from_ + count_; c++)
            {
            	for (Int d = 0; d < Indexes; d++)
            	{
            		node->map().key(d, c) = 0;
            	}

                node->map().data(c) = 0;
            }

            node->set_children_count(node->map().size());
        }
    };



    class MoveElementsFn
    {
    	Int 		from_;
    	Int 		shift_;
    public:


    	Accumulator result_;

    	MoveElementsFn(Int from, Int shift = 0):
    		from_(from), shift_(shift)
    	{}

    	template <typename Node>
    	void operator()(Node* src, Node* tgt)
    	{
    		Int count = src->children_count() - from_;

    		typename MyType::SumKeysFn sum_fn(from_, count, result_.keys());
    		sum_fn(src);

    		if (tgt->children_count() > 0)
    		{
    			tgt->map().InsertSpace(0, count + shift_);
    		}

    		src->map().CopyTo(&tgt->map(), from_, count, shift_);
    		src->map().Clear(from_, from_ + count);

    		src->inc_size(-count);
    		tgt->inc_size(count + shift_);

    		tgt->map().Clear(0, shift_);

    		src->Reindex();
    		tgt->Reindex();
    	}
    };



MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::InsertBatchName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::InsertSubtree(Iterator& iter, ISubtreeProvider& provider)
{
	InsertSharedData data(provider);

	TreePath& path 		= iter.path();
	Int& idx			= iter.key_idx();

	if (iter.key_idx() == 0)
	{
		//FIXME: use iterator here

		TreePath left = path;

		if (me()->GetPrevNode(left))
		{
			//FIXME arguments
			InsertSubtree(left, left[0].node()->children_count(), path, idx, 0, data);

			me()->FinishPathStep(path, idx);

			me()->AddTotalKeyCount(data.provider.GetTotalKeyCount());
		}
		else
		{
			InsertSubtree(path, idx, data);
		}
	}
	else
	{
		InsertSubtree(path, idx, data);
	}

	iter.KeyNum() += provider.GetTotalKeyCount();

	return data.accumulator;
}



//M_PARAMS
//typename M_TYPE::TreePathItem M_TYPE::SplitPath(TreePath& path, Int level, Int idx)
//{
//	if (level < path.GetSize() - 1)
//	{
//		NodeBaseG& parent = path[level + 1].node();
//
//		if (me()->GetCapacity(parent) == 0)
//		{
//			Int idx_in_parent = path[level].parent_idx();
//
//			SplitPath(path, level + 1, idx_in_parent + 1);
//		}
//
//		return Split(path, level, idx);
//	}
//	else
//	{
//		NewRoot(path);
//
//		return Split(path, level, idx);
//	}
//}


M_PARAMS
void M_TYPE::SplitPath(TreePath& left, TreePath& right, Int level, Int idx)
{
	if (level < left.GetSize() - 1)
	{
		NodeBaseG& parent = left[level + 1].node();

		if (me()->GetCapacity(parent) == 0)
		{
			Int idx_in_parent = left[level].parent_idx();
			SplitPath(left, right, level + 1, idx_in_parent + 1);
		}

		Split(left, right, level, idx);
	}
	else
	{
		NewRoot(left);

		right.Resize(left.GetSize());
		right[level + 1] = left[level + 1];

		Split(left, right, level, idx);
	}
}


M_PARAMS
void M_TYPE::InsertEntry(Iterator &iter, const Element& element)
{
	TreePath& 	path 	= iter.path();
	NodeBaseG& 	node 	= path.leaf().node();
	Int& 		idx 	= iter.key_idx();

	if (me()->GetCapacity(node) > 0)
	{
		MakeRoom(path, 0, idx, 1);
	}
	else if (idx == 0)
	{
		TreePath next = path;
		SplitPath(path, next, 0, node->children_count() / 2);
		idx = 0;
		MakeRoom(path, 0, idx, 1);
	}
	else if (idx < node->children_count())
	{
		//FIXME: does it necessary to split the page at the middle ???
		TreePath next = path;
		SplitPath(path, next, 0, idx);
		MakeRoom(path, 0, idx, 1);
	}
	else {
		TreePath next = path;

		SplitPath(path, next, 0, node->children_count() / 2);

		path = next;

		idx = node->children_count();
		MakeRoom(path, 0, idx, 1);

		iter.key_idx() 	= idx;
	}

	me()->SetLeafDataAndReindex(node, idx, element);

	me()->UpdateParentIfExists(path, 0, element.first);

	me()->AddTotalKeyCount(1);
}



//M_PARAMS
//void M_TYPE::InsertEntry(Iterator &iter, Key key, const Value &value) {
//	Accumulator keys;
//
//	keys.key(0) = key;
//
//	InsertEntry(iter, keys, value);
//}

M_PARAMS
void M_TYPE::InsertBatch(Iterator& iter, const LeafNodeKeyValuePair* pairs, BigInt size)
{
	ArraySubtreeProvider provider(*me(), size, pairs);

	me()->InsertSubtree(iter, provider);
}

M_PARAMS
void M_TYPE::InsertBatch(Iterator& iter, const LeafPairsVector& pairs)
{
	ArraySubtreeProvider provider(*me(), pairs.size(), &pairs.at(0));

	me()->InsertSubtree(iter, provider);
}


M_PARAMS
BigInt M_TYPE::GetSubtreeSize(Int level) const
{
	BigInt result = 1;

	for (int c = 0; c < level; c++)
	{
		BigInt children_at_level = me()->GetMaxKeyCountForNode(false, c == 0, c);

		result *= children_at_level;
	}

	return result;
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::GetCounters(const NodeBaseG& node, Int from, Int count) const
{
	Accumulator counters;

	me()->SumKeys(node, from, count, counters.keys());

	return counters;
}



M_PARAMS
void M_TYPE::UpdateUp(TreePath& path, Int level, Int idx, const Accumulator& counters)
{
	for (Int c = level; c < path.GetSize(); c++)
	{
		if (UpdateCounters(path[c].node(), idx, counters))
		{
			break;
		}
		else {
			idx = path[c].parent_idx();
		}
	}
}

M_PARAMS
void M_TYPE::UpdateParentIfExists(TreePath& path, Int level, const Accumulator& counters)
{
	if (level < path.GetSize() - 1)
	{
		me()->UpdateUp(path, level + 1, path[level].parent_idx(), counters);
	}
}





//// ======================================================  PRIVATE API ====================================================== ////



M_PARAMS
void M_TYPE::InsertSubtree(TreePath& left_path, Int left_idx, TreePath& right_path, Int& right_idx, Int level, InsertSharedData& data)
{
	//FIXME: Check node->level() after deletion;

	BigInt 	subtree_size 	= me()->GetSubtreeSize(level);

	BigInt  key_count		= data.total - data.start - data.end;

	NodeBaseG& left_node 	= left_path[level].node();
	NodeBaseG& right_node 	= right_path[level].node();

	if (left_node == right_node)
	{
		Int node_capacity = me()->GetCapacity(left_node);

		if (key_count <= subtree_size * node_capacity)
		{
			// We have enough free space for all subtrees in the current node
			BigInt total 	= Divide(key_count, subtree_size);

			FillNodeLeft(left_path, level, left_idx, total, data);

			right_path.MoveRight(level - 1, 0, total);

			right_idx 		+= total;
		}
		else
		{
			// There is no enough free space in current node for all subtrees
			// Split the node and proceed

			//FIXME:
			me()->SplitPath(left_path, right_path, level, left_idx);

			right_idx  = 0;

			InsertSubtree(left_path, left_idx, right_path, right_idx, level, data);
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

			Int left_count, right_count;

			if (total_keys <= total_capacity)
			{
				Int left_usage 	= left_path[level]->children_count();
				Int right_usage = right_path[level]->children_count();

				if (left_usage < right_usage)
				{
					left_count 	= start_capacity > total_keys ? total_keys : start_capacity;
					right_count = total_keys - left_count;
				}
				else {
					right_count	= end_capacity > total_keys ? total_keys : end_capacity;
					left_count	= total_keys - right_count;
				}
			}
			else {
				left_count 	= start_capacity;
				right_count = end_capacity;
			}

			FillNodeLeft(left_path, level,   left_idx,  left_count,  data);
			FillNodeRight(right_path, level, right_idx, right_count, data);

			right_idx = right_count;
		}
		else
		{
			// If there are more keys than these nodes can store,
			// fill nodes fully

			FillNodeLeft(left_path,   level, left_idx,  start_capacity,  data);
			FillNodeRight(right_path, level, right_idx, end_capacity, data);

			right_idx += end_capacity;

			// There is something more to insert.
			Int parent_right_idx = right_path[level].parent_idx();
			InsertSubtree
			(
					left_path,
					left_path[level].parent_idx() + 1,
					right_path,
					parent_right_idx,
					level + 1,
					data
			);
		}
	}
}






M_PARAMS
void M_TYPE::FillNodeLeft(TreePath& path, Int level, Int from, Int count, InsertSharedData& data)
{
	NodeBaseG& node = path[level].node();

	BigInt subtree_size = me()->GetSubtreeSize(level);

	MakeRoom(path, level, from, count);

	if (level > 0)
	{
		for (Int c = from; c < from + count; c++)
		{
			BigInt 					requested_size 	= data.remains >= subtree_size ? subtree_size : data.remains;
			NonLeafNodeKeyValuePair pair 			= data.provider.GetKVPair(ISubtreeProvider::FORWARD, data.start, requested_size, level);

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

	me()->UpdateParentIfExists(path, level, accumulator);

	data.accumulator += accumulator;
}


M_PARAMS
void M_TYPE::FillNodeRight(TreePath& path, Int level, Int from, Int count, InsertSharedData& data)
{
	NodeBaseG& 	node 			= path[level].node();
	BigInt 		subtree_size 	= me()->GetSubtreeSize(level);

	MakeRoom(path, level, from, count);

	if (level > 0)
	{
		for (Int c = count - 1; c >= 0; c--)
		{
			BigInt 					requested_size 	= data.remains >= subtree_size ? subtree_size : data.remains;
			NonLeafNodeKeyValuePair pair 			= data.provider.GetKVPair(ISubtreeProvider::BACKWARD, data.end, requested_size, level);

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


// FIXME: ??????? why is this here?
//	if (level > 0) {
//		path[level-1].parent_idx() += count;
//	}

	ReindexAndUpdateCounters(node);

	Accumulator accumulator = me()->GetCounters(node, from, count);

	me()->UpdateParentIfExists(path, level, accumulator);

	data.accumulator += accumulator;
}

M_PARAMS
void M_TYPE::MakeRoom(TreePath& path, Int level, Int start, Int count) const
{
	path[level].node().update();

	MakeRoomFn fn(start, count);
	NodeDispatcher::Dispatch(path[level].node(), fn);

	path.MoveRight(level - 1, start, count);

//	if (level > 0)
//	{
//		Int& parent_idx = path[level - 1].parent_idx();
//
//		if (parent_idx >=  (start + count))
//		{
//			parent_idx += count;
//		}
//	}
}

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::MoveElements(NodeBaseG& src, NodeBaseG& tgt, Int from, Int shift) const
{
	MoveElementsFn fn(from, shift);
	NodeDispatcher::Dispatch(src, tgt, fn);

	return fn.result_;
}


M_PARAMS
bool M_TYPE::UpdateCounters(NodeBaseG& node, Int idx, const Accumulator& counters) const
{
	node.update();
	me()->AddKeys(node, idx, counters.keys());

	return false; //proceed further unconditionally
}


M_PARAMS
typename M_TYPE::TreePathItem M_TYPE::Split(TreePath& path, Int level, Int idx)
{
	NodeBaseG& node 	= path[level].node();
	NodeBaseG& parent 	= path[level + 1].node();

	Int parent_idx		= path[level].parent_idx();

	node.update();
	parent.update();

	NodeBaseG other = me()->CreateNode(level, false, node->is_leaf());

	Accumulator keys = MoveElements(node, other, idx);

	//FIXME:: Make room in the parent
	MakeRoom(path, level + 1, parent_idx + 1, 1);

	me()->SetINodeData(parent, parent_idx + 1, &other->id());

	//FIXME: Should we proceed up to the root here in general case?
	UpdateCounters(parent, parent_idx, 	  -keys);
	UpdateCounters(parent, parent_idx + 1, keys);

	if (level > 0)
	{
		if (path[level - 1].parent_idx() < idx)
		{
			return TreePathItem(other, parent_idx + 1);
		}
		else {

			TreePathItem item = path[level];

			path[level    ].node() 			= other;
			path[level    ].parent_idx()++;

			path[level - 1].parent_idx() 	-= idx;

			return item;
		}
	}
	else {
		return TreePathItem(other, parent_idx + 1);
	}
}

M_PARAMS
void M_TYPE::Split(TreePath& left, TreePath& right, Int level, Int idx)
{
	NodeBaseG& left_node 	= left[level].node();

	NodeBaseG& left_parent 	= left[level + 1].node();
	NodeBaseG& right_parent = right[level + 1].node();

	left_node.update();
	left_parent.update();
	right_parent.update();

	NodeBaseG other = me()->CreateNode(level, false, left_node->is_leaf());

	Accumulator keys = MoveElements(left_node, other, idx);

	Int parent_idx = left[level].parent_idx();

	if (right_parent == left_parent)
	{
		MakeRoom(left, level + 1,  parent_idx + 1, 1);
		me()->SetINodeData(left_parent, parent_idx + 1, &other->id());

		//FIXME: should we proceed up to the root?
		UpdateCounters(left_parent, parent_idx,    -keys);
		UpdateCounters(left_parent, parent_idx + 1, keys);

		right[level].node() 		= other;
		right[level].parent_idx() 	= parent_idx + 1;
	}
	else {
		MakeRoom(right, level + 1, 0, 1);
		me()->SetINodeData(right_parent, 0, &other->id());

		UpdateUp(left,  level + 1, parent_idx, -keys);
		UpdateUp(right, level + 1, 0,  			keys);

		right[level].node() 		= other;
		right[level].parent_idx() 	= 0;
	}

	right.MoveLeft(level - 1, 0, idx);

//	if (level > 0)
//	{
//		right[level-1].parent_idx() -= idx;
//	}
}

M_PARAMS
void M_TYPE::NewRoot(TreePath& path)
{
	NodeBaseG& root 		= path[path.GetSize() - 1].node(); // page == root
	root.update();

	NodeBaseG new_root 		= me()->CreateNode(root->level() + 1, true, false);

	me()->CopyRootMetadata(root, new_root);

	me()->Root2Node(root);

	Accumulator keys = me()->GetMaxKeys(root);
	me()->SetKeys(new_root, 0, keys);
	me()->SetINodeData(new_root, 0, &root->id());
	me()->SetChildrenCount(new_root, 1);
	me()->Reindex(new_root);

	me()->set_root(new_root->id());

	path.Append(TreePathItem(new_root));

	path[path.GetSize() - 2].parent_idx() = 0;
}

#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
