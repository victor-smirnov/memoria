
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_BATCH_COMMON_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_BATCH_COMMON_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>
#include <algorithm>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::InsertBatchCommonName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    typedef typename Types::Source                                              Source;


    class Checkpoint {
    	NodeBaseG head_;
    	Int size_;
    public:
    	Checkpoint(NodeBaseG head, Int size): head_(head), size_(size) {}

    	NodeBaseG head() const {return head_;};
    	Int size() const {return size_;};
    };


    struct ILeafProvider {
    	virtual NodeBaseG get_leaf() 	= 0;

    	virtual Checkpoint checkpoint() = 0;

    	virtual void rollback(const Checkpoint& chekpoint) = 0;

    	virtual CtrSizeT size() const		= 0;
    };

//    class Subtree {
//    	NodeBaseG node_;
//    	CtrSizeT size_;
//
//    public:
//    	Subtree(NodeBaseG node, CtrSizeT size): node_(node), size_(size) {}
//    	Subtree(): size_(0) {}
//
//    	NodeBaseG node() {
//    		return node_;
//    	}
//
//    	const NodeBaseG node() const {
//    		return node_;
//    	}
//
//    	CtrSizeT size() const {
//    		return size_;
//    	}
//    };

    void updateChildIndexes(NodeBaseG& node, Int start)
    {
    	auto& self = this->self();
		Int size = self.getBranchNodeSize(node);

		if (start < size)
		{
			self.forAllIDs(node, start, size, [&, this](const ID& id, Int parent_idx)
			{
				auto& self = this->self();
				NodeBaseG child = self.allocator().getPageForUpdate(id, self.master_name());

				child->parent_idx() = parent_idx;
			});
		}
    }

    void remove_branch_nodes(ID node_id)
    {
        auto& self = this->self();

        NodeBaseG node = self.allocator().getPage(node_id, self.master_name());

        if (node->level() > 0)
        {
            self.forAllIDs(node, [&, this](const ID& id, Int idx)
            {
                auto& self = this->self();
                self.remove_branch_nodes(id);
            });

            self.allocator().removePage(node->id(), self.master_name());
        }
    }

    class InsertBatchResult {
    	Int idx_;
    	CtrSizeT subtree_size_;
    public:
    	InsertBatchResult(Int idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

    	Int idx() const {return idx_;}
    	CtrSizeT subtree_size() const {return subtree_size_;}
    };


    NodeBaseG BuildSubtree(ILeafProvider& provider, Int level)
    {
    	auto& self = this->self();

    	if (provider.size() > 0)
    	{
    		if (level >= 1)
    		{
    			NodeBaseG node = self.createNode1(level, false, false);

    			self.layoutNonLeafNode(node, 0xFF);

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




//    class InsertionState {
//    	Int inserted_ = 0;
//    	Int total_;
//    public:
//    	InsertionState(Int total): total_(total) {}
//
//    	Int& total() {
//    		return total_;
//    	}
//
//    	Int& inserted() {
//    		return inserted_;
//    	}
//
//    	bool shouldMoveUp() const {
//    		return inserted_ <= total_ / 3;
//    	}
//    };
//
//
//    InsertBatchResult insertBatchToNode(NodeBaseG& node, Int idx, ILeafProvider& provider, Int level = 1, bool update_hierarchy = true)
//    {
//    	auto& self = this->self();
//    	return self.insertSubtree(node, idx, provider, [&provider, &node, this]() -> NodeBaseG {
//    		auto& self = this->self();
//    		return self.BuildSubtree(provider, node->level() - 1);
//    	},
//    	update_hierarchy);
//    }
//
//    void insert_subtree(NodeBaseG& left, NodeBaseG& right, ILeafProvider& provider, InsertionState& state, Int level = 1)
//    {
//    	auto& self = this->self();
//
//    	Int left_size0 = self.getBranchNodeSize(left);
//
//    	auto left_result = insertBatchToNode(left, left_size0, provider, level);
//
//    	state.inserted() += left_result.subtree_size();
//
//    	if (state.shouldMoveUp())
//    	{
//    		auto left_parent 	= self.getNodeParentForUpdate(left);
//    		auto right_parent 	= self.getNodeParentForUpdate(right);
//
//    		if (left_parent == right_parent)
//    		{
//    			right_parent = self.splitPathP(left_parent, right->parent_idx());
//    		}
//
//    		insert_subtree(left_parent, right_parent, provider, state, level + 1);
//    	}
//    	else {
//    		auto right_result = insertBatchToNode(right, 0, provider, level);
//    		state.inserted() += right_result.subtree_size();
//    	}
//    }
//
//    NodeBaseG insert_subtree_at_end(NodeBaseG& left, ILeafProvider& provider, InsertionState& state, Int level = 1)
//    {
//    	auto& self = this->self();
//
//    	Int left_size0 = self.getBranchNodeSize(left);
//
//    	auto left_result = insertBatchToNode(left, left_size0, provider, level);
//
//    	state.inserted() += left_result.subtree_size();
//
//    	if (provider.size() > 0)
//    	{
//    		if (left->is_root())
//    		{
//    			self.newRootP(left);
//    		}
//
//    		auto left_parent = self.getNodeParentForUpdate(left);
//
//    		auto right = insert_subtree_at_end(left_parent, provider, state, level + 1);
//
//    		Int right_size = self.getBranchNodeSize(right);
//
//    		return self.getChild(right, right_size - 1);
//    	}
//    	else {
//    		return left;
//    	}
//    }
//
//
//    Int insert_branch_subtree(NodeBaseG& node, Int pos, ILeafProvider& provider)
//    {
//    	auto& self = this->self();
//
//    	auto result = insertBatchToNode(node, pos, provider);
//
//    	if (provider.size() == 0)
//    	{
//    		return result.idx();
//    	}
//    	else {
//    		auto node_size = self.getBranchNodeSize(node);
//
//    		NodeBaseG next;
//
//    		if (result.idx() < node_size)
//    		{
//    			next = self.splitPathP(node, result.idx());
//    		}
//    		else {
//    			next = self.getNextNodeP(node);
//    		}
//
//    		if (next.isSet())
//    		{
//    			auto left_result = insertBatchToNode(node, result.idx(), provider);
//
//    			if (provider.size() == 0)
//    			{
//    				return left_result.idx();
//    			}
//    			else {
//    				PageUpdateMgr mgr(self);
//    				mgr.add(next);
//
//    				auto next_result = insertBatchToNode(next, 0, provider, 1, false);
//
//    				if (provider.size() == 0)
//    				{
//    					auto sums = self.sums(next, 0, next_result.idx());
//    					self.updateParent(next, sums);
//
//    					updateChildIndexes(next, next_result.idx());
//
//    					node = next;
//
//    					return next_result.idx();
//    				}
//    				else {
//    					mgr.rollback();
//
//    					InsertionState state(provider.size());
//
//    					auto next_size0 = self.getBranchNodeSize(next);
//
//    					insert_subtree(node, next, provider, state);
//
//    					auto idx = self.getBranchNodeSize(next) - next_size0;
//
//    					if (provider.size() == 0)
//    					{
//    						node = next;
//    						return idx;
//    					}
//    					else {
//    						return insert_subtree(next, idx, provider);
//    					}
//    				}
//    			}
//    		}
//    		else {
//    			InsertionState state(provider.size());
//    			node = insert_subtree_at_end(node, provider, state, 1);
//
//    			return self.getBranchNodeSize(node);
//    		}
//    	}
//    }

    std::pair<CtrSizeT, NodeBaseG> createLeafList2(Source& source);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::InsertBatchCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
std::pair<typename M_TYPE::CtrSizeT, typename M_TYPE::NodeBaseG> M_TYPE::createLeafList2(Source& source)
{
    auto& self = this->self();

    CtrSizeT    total   = 0;
    NodeBaseG   head;
    NodeBaseG   current;

    Int page_size = self.getRootMetadata().page_size();

    while (true)
    {
        Position remainder = self.getRemainderSize(source);

        if (remainder.gtAny(0))
        {
            total++;

            NodeBaseG node = self.createNode1(0, false, true, page_size);

            self.fillNewLeaf(node, source);

            if (head.isSet())
            {
                current->next_leaf_id() = node->id();
                current                 = node;
            }
            else {
                head = current = node;
            }
        }
        else {
            break;
        }
    }

    return std::make_pair(total, head);
}



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
