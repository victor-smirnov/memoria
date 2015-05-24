
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_BATCH_FIXED_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_BATCH_FIXED_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>
#include <algorithm>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::InsertBatchFixedName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    typedef typename Types::Source                                              Source;



    using Checkpoint 	= typename Base::Checkpoint;
    using ILeafProvider = typename Base::ILeafProvider;



    class InsertBatchResult {
    	Int idx_;
    	CtrSizeT subtree_size_;
    public:
    	InsertBatchResult(Int idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

    	Int idx() const {return idx_;}
    	CtrSizeT subtree_size() const {return subtree_size_;}
    };

    class BranchNodeEntry {
    	Accumulator accum_;
    	ID child_id_;
    public:
    	BranchNodeEntry(const Accumulator& accum, const ID& id): accum_(accum), child_id_(id) {}
    	BranchNodeEntry() : child_id_(0) {}

    	const Accumulator& accum() const {return accum_;}
    	const ID& child_id() const {return child_id_;}

    	Accumulator& accum() {return accum_;}
    	ID& child_id() {return child_id_;}
    };


    struct InsertChildrenFn {
    	template <typename NodeTypes>
    	void treeNode(BranchNode<NodeTypes>* node, Int from, Int to, const BranchNodeEntry* entries)
    	{
    		int old_size = node->size();

    		node->processAll(*this, from, to, entries);

    		Int idx = 0;
    		node->insertValues(old_size, from, to - from, [entries, &idx](){
    			return entries[idx++].child_id();
    		});
    	}

    	template <Int ListIdx, typename StreamType>
    	void stream(StreamType* obj, Int from, Int to, const BranchNodeEntry* entries)
    	{
    		Int idx = 0;
    		obj->insert(from, to - from, [entries, &idx]() {
    			return std::get<ListIdx>(entries[idx++].accum());
    		});
    	}
    };


    MEMORIA_DECLARE_NODE_FN(ReindexBranchNodeFn, reindex);

    InsertBatchResult insertSubtree(NodeBaseG& node, Int idx, ILeafProvider& provider, std::function<NodeBaseG ()> child_fn, bool update_hierarchy)
    {
    	auto& self = this->self();

    	Int capacity 			= self.getCapacity(node);
    	CtrSizeT provider_size0 = provider.size();
    	const Int batch_size 	= 32;

    	Int max = idx;

    	for (Int c = 0; c < capacity; c+= batch_size)
    	{
    		BranchNodeEntry subtrees[batch_size];

    		Int i, batch_max = (c + batch_size) < capacity ? batch_size : (capacity - c);
    		for (i = 0; i < batch_max && provider.size() > 0; i++)
    		{
    			NodeBaseG child = child_fn();

    			subtrees[i].accum() 	= self.sums(child);
    			subtrees[i].child_id() 	= child->id();

    			child->parent_id() = node->id();
    			child->parent_idx() = idx + c + i;
    		}

    		BranchDispatcher::dispatch(node, InsertChildrenFn(), idx + c, idx + c + i, subtrees);

    		max = idx + c + i;

    		if (i < batch_max)
    		{
    			break;
    		}
    	}

    	if (update_hierarchy)
    	{
    		Accumulator sums = self.sums(node, idx, max);
    		self.updateParent(node, sums);
    		self.updateChildIndexes(node, max);
    	}

    	return InsertBatchResult(max, provider_size0 - provider.size());
    }


    NodeBaseG BuildSubtree(ILeafProvider& provider, Int level)
    {
    	auto& self = this->self();

    	if (provider.size() > 0)
    	{
    		if (level >= 1)
    		{
    			NodeBaseG node = self.createNode1(level, false, false);

    			self.layoutBranchNode(node, 0xFF);

    			self.insertSubtree(node, 0, provider, [this, level, &provider]() -> NodeBaseG {
    				auto& self = this->self();
    				return self.BuildSubtree(provider, level - 1);
    			}, false);

    			return node;
    		}
    		else {
    			return provider.get_leaf();
    		}
    	}
    	else {
    		return NodeBaseG();
    	}
    }





    class ListLeafProvider: public ILeafProvider {
    	NodeBaseG	head_;
    	CtrSizeT 	size_ = 0;

    	MyType& 	ctr_;

    public:
    	ListLeafProvider(MyType& ctr, NodeBaseG head, CtrSizeT size): head_(head),  size_(size), ctr_(ctr) {}

    	virtual CtrSizeT size() const
    	{
    		return size_;
    	}

    	virtual NodeBaseG get_leaf()
    	{
    		if (head_.isSet())
    		{
    			auto node = head_;
    			head_ = ctr_.allocator().getPage(head_->next_leaf_id(), ctr_.master_name());
    			size_--;
    			return node;
    		}
    		else {
    			throw memoria::vapi::BoundsException(MA_SRC, "Leaf List is empty");
    		}
    	}


    	virtual Checkpoint checkpoint() {
    		return Checkpoint(head_, size_);
    	}


    	virtual void rollback(const Checkpoint& checkpoint)
    	{
    		size_ 	= checkpoint.size();
    		head_ 	= checkpoint.head();
    	}
    };




    class InsertionState {
    	Int inserted_ = 0;
    	Int total_;
    public:
    	InsertionState(Int total): total_(total) {}

    	Int& total() {
    		return total_;
    	}

    	Int& inserted() {
    		return inserted_;
    	}

    	bool shouldMoveUp() const {
    		return inserted_ <= total_ / 3;
    	}
    };


    InsertBatchResult insertBatchToNode(NodeBaseG& node, Int idx, ILeafProvider& provider, Int level = 1, bool update_hierarchy = true)
    {
    	auto& self = this->self();
    	return self.insertSubtree(node, idx, provider, [&provider, &node, this]() -> NodeBaseG {
    		auto& self = this->self();
    		return self.BuildSubtree(provider, node->level() - 1);
    	},
    	update_hierarchy);
    }

    void insert_subtree(NodeBaseG& left, NodeBaseG& right, ILeafProvider& provider, InsertionState& state, Int level = 1)
    {
    	auto& self = this->self();

    	Int left_size0 = self.getBranchNodeSize(left);

    	auto left_result = insertBatchToNode(left, left_size0, provider, level);

    	state.inserted() += left_result.subtree_size();

    	if (state.shouldMoveUp())
    	{
    		auto left_parent 	= self.getNodeParentForUpdate(left);
    		auto right_parent 	= self.getNodeParentForUpdate(right);

    		if (left_parent == right_parent)
    		{
    			right_parent = self.splitPathP(left_parent, right->parent_idx());
    		}

    		insert_subtree(left_parent, right_parent, provider, state, level + 1);
    	}
    	else {
    		auto right_result = insertBatchToNode(right, 0, provider, level);
    		state.inserted() += right_result.subtree_size();
    	}
    }

    NodeBaseG insert_subtree_at_end(NodeBaseG& left, ILeafProvider& provider, InsertionState& state, Int level = 1)
    {
    	auto& self = this->self();

    	Int left_size0 = self.getBranchNodeSize(left);

    	auto left_result = insertBatchToNode(left, left_size0, provider, level);

    	state.inserted() += left_result.subtree_size();

    	if (provider.size() > 0)
    	{
    		if (left->is_root())
    		{
    			self.newRootP(left);
    		}

    		auto left_parent = self.getNodeParentForUpdate(left);

    		auto right = insert_subtree_at_end(left_parent, provider, state, level + 1);

    		Int right_size = self.getBranchNodeSize(right);

    		return self.getChild(right, right_size - 1);
    	}
    	else {
    		return left;
    	}
    }


    Int insert_subtree(NodeBaseG& node, Int pos, ILeafProvider& provider)
    {
    	auto& self = this->self();

    	auto result = insertBatchToNode(node, pos, provider);

    	if (provider.size() == 0)
    	{
    		return result.idx();
    	}
    	else {
    		auto node_size = self.getBranchNodeSize(node);

    		NodeBaseG next;

    		if (result.idx() < node_size)
    		{
    			next = self.splitPathP(node, result.idx());
    		}
    		else {
    			next = self.getNextNodeP(node);
    		}

    		if (next.isSet())
    		{
    			auto left_result = insertBatchToNode(node, result.idx(), provider);

    			if (provider.size() == 0)
    			{
    				return left_result.idx();
    			}
    			else {
    				InsertionState state(provider.size());

    				auto next_size0 = self.getBranchNodeSize(next);

    				insert_subtree(node, next, provider, state);

    				auto idx = self.getBranchNodeSize(next) - next_size0;

    				if (provider.size() == 0)
    				{
    					node = next;
    					return idx;
    				}
    				else {
    					return insert_subtree(next, idx, provider);
    				}
    			}
    		}
    		else {
    			InsertionState state(provider.size());
    			node = insert_subtree_at_end(node, provider, state, 1);

    			return self.getBranchNodeSize(node);
    		}
    	}
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::InsertBatchFixedName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
