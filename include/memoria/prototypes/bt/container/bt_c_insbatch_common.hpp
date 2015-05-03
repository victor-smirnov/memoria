
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





    std::pair<CtrSizeT, NodeBaseG> createLeafList2(Source& source);




    Accumulator insertSource(NodeBaseG& node, Position& idx, Source& source);

    MEMORIA_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, Accumulator);
    Accumulator splitLeafNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at);
    Accumulator splitNonLeafNode(NodeBaseG& src, NodeBaseG& tgt, Int split_at);

    Accumulator splitNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at)
    {
        auto& self = this->self();

        if (src->is_leaf())
        {
            return self.splitLeafNode(src, tgt, split_at);
        }
        else {
            return self.splitNonLeafNode(src, tgt, split_at.get());
        }
    }

    void newRootP(NodeBaseG& root);


    void updateChildren(const NodeBaseG& node);
    void updateChildren(const NodeBaseG& node, Int start);
    void updateChildren(const NodeBaseG& node, Int start, Int end);

    MEMORIA_DECLARE_NODE_FN_RTN(IsEmptyFn, isEmpty, bool);
    MEMORIA_DECLARE_NODE_FN_RTN(IsAfterEndFn, isAfterEnd, bool);

    bool isEmpty(const NodeBaseG& node) {
        return NodeDispatcher::dispatch(node, IsEmptyFn());
    }

    bool isAfterEnd(const NodeBaseG& node, const Position& idx, UBigInt active_streams)
    {
        return NodeDispatcher::dispatch(node, IsAfterEndFn(), idx, active_streams);
    }

private:
    void updateChildrenInternal(const NodeBaseG& node, Int start, Int end);
public:


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::InsertBatchCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::insertSource(NodeBaseG& leaf, Position& idx, Source& source)
{
    auto& self = this->self();

    Position sizes = self.getRemainderSize(source);

    UBigInt active_streams = sizes.gtZero();

    Accumulator sums;
    if (self.insertToLeaf(leaf, idx, source, sums))
    {
        self.updateParent(leaf, sums);

        return sums;
    }
    else {
        auto right = leaf;

        if (leaf->is_root())
        {
            self.newRootP(leaf);
        }

        if (!self.isAfterEnd(leaf, idx, active_streams))
        {
            right = self.splitLeafP(leaf, idx);
        }

        Accumulator sums = self.appendToLeaf(leaf, idx, source);

        self.updateParent(leaf, sums);

        Position remainder = self.getRemainderSize(source);

        if (remainder.gtAny(0))
        {
            Int path_parent_idx     = leaf->parent_idx() + 1;

            auto pair = self.createLeafList2(source);

            using Provider = ListLeafProvider;

            Provider provider(self, pair.second, pair.first);

            NodeBaseG parent = self.getNodeParentForUpdate(leaf);

            self.insert_subtree(parent, path_parent_idx, provider);

            return sums; //fixme: this accumulator contains data from the forst leaf only
        }
        else {
            return sums;
        }
    }
}



M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node)
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        self.updateChildrenInternal(node, 0, self.getNodeSize(node, 0));
    }
}

M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node, Int start)
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        self.updateChildrenInternal(node, start, self.getNodeSize(node, 0));
    }
}

M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node, Int start, Int end)
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        self.updateChildrenInternal(node, start, end);
    }
}


M_PARAMS
void M_TYPE::updateChildrenInternal(const NodeBaseG& node, Int start, Int end)
{
    auto& self = this->self();

    ID node_id = node->id();

    self.forAllIDs(node, start, end, [&self, &node_id](const ID& id, Int idx)
    {
        NodeBaseG child = self.allocator().getPageForUpdate(id, self.master_name());

        child->parent_id()  = node_id;
        child->parent_idx() = idx;
    });
}




M_PARAMS
void M_TYPE::newRootP(NodeBaseG& root)
{
    auto& self = this->self();

    self.updatePageG(root);

    NodeBaseG new_root = self.createNode1(root->level() + 1, true, false, root->page_size());

    UBigInt root_active_streams = self.getActiveStreams(root);
    self.layoutNonLeafNode(new_root, root_active_streams);

    self.copyRootMetadata(root, new_root);

    self.root2Node(root);

    Accumulator keys = self.sums(root);

    self.insertNonLeaf(new_root, 0, keys, root->id());

    root->parent_id()  = new_root->id();
    root->parent_idx() = 0;

    self.set_root(new_root->id());
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::splitLeafNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at)
{
    return LeafDispatcher::dispatch(src, tgt, SplitNodeFn(), split_at);
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::splitNonLeafNode(NodeBaseG& src, NodeBaseG& tgt, Int split_at)
{
    auto& self = this->self();

    Accumulator accum = NonLeafDispatcher::dispatch(src, tgt, SplitNodeFn(), split_at);

    self.updateChildren(tgt);

    return accum;
}











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
