
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_FIND_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_FIND_HPP

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/core/container/macros.hpp>

#include <limits>

namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::FindName)

    using Types = TypesType;

	using typename Base::Allocator;

    using typename Base::NodeBaseG;
    using typename Base::Iterator;
    using typename Base::IteratorPtr;
    using typename Base::Position;
    using typename Base::CtrSizeT;

    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;



    using LeafStreamsStructList = typename Types::LeafStreamsStructList;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;
    template <typename LeafPath>
    using TargetType2 = typename Types::template TargetType2<LeafPath>;




public:

    template <typename Walker>
    IteratorPtr find_(Walker&& walker);


    template <typename LeafPath>
    IteratorPtr find_gt(Int index, TargetType<LeafPath> key)
    {
    	typename Types::template FindGTForwardWalker<Types, LeafPath> walker(index, key);
    	return self().find_(walker);
    }

    template <typename LeafPath>
    IteratorPtr find_max_gt(Int index, TargetType<LeafPath> key)
    {
    	typename Types::template FindMaxGTWalker<Types, LeafPath> walker(index, key);
    	return self().find_(walker);
    }


    template <typename LeafPath>
    IteratorPtr find_ge(Int index, TargetType<LeafPath> key)
    {
    	typename Types::template FindGEForwardWalker<Types, LeafPath> walker(index, key);
    	return self().find_(walker);
    }

    template <typename LeafPath>
    IteratorPtr find_max_ge(Int index, TargetType<LeafPath> key)
    {
    	typename Types::template FindMaxGEWalker<Types, LeafPath> walker(index, key);
    	return self().find_(walker);
    }

    template <typename LeafPath>
    IteratorPtr rank_(Int index, CtrSizeT pos)
    {
    	typename Types::template RankForwardWalker<Types, LeafPath> walker(index, pos);
    	return self().find_(walker);
    }

    template <typename LeafPath>
    IteratorPtr select_(Int index, CtrSizeT rank)
    {
    	typename Types::template SelectForwardWalker<Types, LeafPath> walker(index, rank);
    	return self().find_(walker);
    }


    struct NodeChain {
    	NodeBaseG node;
    	Int start;
    	Int end;
    	NodeChain* ref;

    	NodeChain(NodeBaseG _node, Int _start, NodeChain* _ref = nullptr): node(_node), start(_start), end(0), ref(_ref) {}

    	void swapRanges()
    	{
    		auto tmp = start;
    		end = start;
    		start = tmp;
    	}

    	template <typename Walker>
    	WalkCmd processChain(Walker&& walker, Int leaf_cnt = 0)
    	{
    		if (node->is_leaf())
    		{
    			leaf_cnt++;
    		}

    		if (ref)
    		{
    			ref->processChain(std::forward<Walker>(walker), leaf_cnt);
    		}

    		if (node->is_leaf())
    		{
    			WalkCmd cmd;

        		if (leaf_cnt == 1)
        		{
        			if (ref == nullptr) {
        				cmd = WalkCmd::THE_ONLY_LEAF;
        			}
        			else {
        				cmd = WalkCmd::LAST_LEAF;
        			}
        		}
        		else if (leaf_cnt == 2) {
        			cmd = WalkCmd::FIRST_LEAF;
        		}

        		LeafDispatcher::dispatch(node, std::forward<Walker>(walker), cmd, start, end);

        		return cmd;
    		}
    		else {
    			BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::PREFIXES, start, end);

    			return WalkCmd::PREFIXES;
    		}
    	}
    };

    struct FindResult {
    	NodeBaseG 	node;
    	Int 		idx;
    	bool 		pass;
    	WalkCmd 	cmd;

    	explicit FindResult(NodeBaseG _node, Int _idx, WalkCmd _cmd, bool _pass = true): node(_node), idx(_idx), pass(_pass), cmd(_cmd) {}
    };

    template <typename Walker>
    StreamOpResult find_fw(NodeBaseG& node, Int stream, Int idx, Walker&& walker);

    template <typename Walker>
    FindResult find_fw(NodeChain node_chain, Walker&& walker, WalkDirection direction = WalkDirection::UP);


    template <typename Walker>
    FindResult find_bw(NodeChain node_chain, Walker&& walker, WalkDirection direction = WalkDirection::UP);

    template <Int Stream>
    IteratorPtr seek_stream(CtrSizeT position)
    {
    	typename Types::template SkipForwardWalker<Types, IntList<Stream>> walker(position);
    	return self().find_(walker);
    }


    MEMORIA_DECLARE_NODE_FN_RTN(SizesFn, size_sums, Position);
    Position sizes() const
    {
    	NodeBaseG node = self().getRoot();
    	return NodeDispatcher::dispatch(node, SizesFn());
    }

    template <typename Walker>
    void walkUp(NodeBaseG node, Int idx, Walker&& walker) const
    {
    	if (node->is_leaf())
    	{
    		LeafDispatcher::dispatch(node, walker, WalkCmd::LAST_LEAF, 0, idx);
    	}
    	else {
    		BranchDispatcher::dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);
    	}

    	while (!node->is_root())
    	{
    		idx = node->parent_idx();
    		node = self().getNodeParent(node);

    		NodeDispatcher::dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);
    	}
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::FindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Walker>
typename M_TYPE::FindResult M_TYPE::find_fw(NodeChain node_chain, Walker&& walker, WalkDirection direction)
{
    auto& self = this->self();

    auto start 		 = node_chain.start;
    const auto& node = node_chain.node;

    auto result = NodeDispatcher::dispatch(node, std::forward<Walker>(walker), direction, start);
    node_chain.end = result.idx();

    if (direction == WalkDirection::UP)
    {
    	if (!result.out_of_range())
    	{
    		if (node->is_leaf())
    		{
    			auto cmd = node_chain.processChain(std::forward<Walker>(walker));
    			return FindResult(node, result.idx(), cmd);
    		}
    		else {
    			auto child = self.getChild(node, result.idx());
    			return find_fw(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    		}
    	}
    	else {
    		if (!node_chain.node->is_root())
    		{
    			auto parent 		= self.getNodeParent(node);
    			auto parent_idx 	= node->parent_idx() + 1;
    			auto parent_result  = find_fw(NodeChain(parent, parent_idx, &node_chain), std::forward<Walker>(walker), WalkDirection::UP);

    			if (parent_result.pass)
    			{
    				return parent_result;
    			}
    		}

    		if (node->is_leaf())
    		{
    			auto cmd = node_chain.processChain(std::forward<Walker>(walker));
    			return FindResult(node, result.idx(), cmd);
    		}
    		else if (!result.empty())
    		{
    			BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.idx() - 1);
    			node_chain.end = result.idx() - 1;

    			auto child = self.getChild(node, result.idx() - 1);
    			return find_fw(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    		}
    		else {
    			return FindResult(node, start, WalkCmd::NONE, false);
    		}
    	}
    }
    else if (node_chain.node->is_leaf())
    {
    	auto cmd = node_chain.processChain(std::forward<Walker>(walker));
    	return FindResult(node_chain.node, result.idx(), cmd);
    }
    else if (!result.out_of_range())
    {
    	auto child = self.getChild(node_chain.node, result.idx());
    	return find_fw(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }
    else
    {
    	BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.idx() - 1);
    	node_chain.end = result.idx() - 1;

    	auto child = self.getChild(node_chain.node, result.idx() - 1);
    	return find_fw(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }

}




M_PARAMS
template <typename Walker>
typename M_TYPE::FindResult M_TYPE::find_bw(NodeChain node_chain, Walker&& walker, WalkDirection direction)
{
    auto& self = this->self();

    auto result = NodeDispatcher::dispatch(node_chain.node, std::forward<Walker>(walker), direction, node_chain.start);
    node_chain.end = result.idx();

    const Int max = std::numeric_limits<Int>::max() - 2;

    if (direction == WalkDirection::UP)
    {
    	if (!result.out_of_range())
    	{
    		if (node_chain.node->is_leaf())
    		{
    			auto cmd = node_chain.processChain(std::forward<Walker>(walker));
    			return FindResult(node_chain.node, result.idx(), cmd);
    		}
    		else {
    			auto child = self.getChild(node_chain.node, result.idx());
    			return find_bw(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    		}
    	}
    	else {
    		if (!node_chain.node->is_root())
    		{
    			auto parent 		= self.getNodeParent(node_chain.node);
    			auto parent_idx 	= node_chain.node->parent_idx() - 1;
    			auto parent_result  = find_bw(NodeChain(parent, parent_idx, &node_chain), std::forward<Walker>(walker), WalkDirection::UP);

    			if (parent_result.pass)
    			{
    				return parent_result;
    			}
    		}

    		if (node_chain.node->is_leaf())
    		{
    			auto cmd = node_chain.processChain(std::forward<Walker>(walker));
    			return FindResult(node_chain.node, result.idx(), cmd);
    		}
    		else if (!result.empty())
    		{
    			BranchDispatcher::dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.idx());
    			node_chain.end = result.idx();

    			auto child = self.getChild(node_chain.node, result.idx() + 1);
    			return find_bw(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    		}
    		else {
    			return FindResult(node_chain.node, node_chain.start, WalkCmd::NONE, false);
    		}
    	}
    }
    else if (node_chain.node->is_leaf())
    {
    	auto cmd = node_chain.processChain(std::forward<Walker>(walker));
    	return FindResult(node_chain.node, result.idx(), cmd);
    }
    else if (!result.out_of_range())
    {
    	auto child = self.getChild(node_chain.node, result.idx());
    	return find_bw(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }
    else
    {
    	BranchDispatcher::dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.idx());
    	node_chain.end = result.idx();

    	auto child = self.getChild(node_chain.node, result.idx() + 1);
    	return find_bw(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }

}





M_PARAMS
template <typename Walker>
typename M_TYPE::IteratorPtr M_TYPE::find_(Walker&& walker)
{
    auto& self = this->self();

    IteratorPtr i = self.make_iterator(self);

    NodeBaseG node = self.getRoot();
    if (node.isSet())
    {
        while (!node->is_leaf())
        {
        	auto result = BranchDispatcher::dispatch(node, walker, WalkDirection::DOWN, 0);
        	Int idx = result.idx();

        	if (result.out_of_range())
        	{
        		idx--;
        		BranchDispatcher::dispatch(node, walker, WalkCmd::FIX_TARGET, 0, idx);
        	}

        	BranchDispatcher::dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);

        	node = self.getChild(node, idx);
        }

        auto result = LeafDispatcher::dispatch(node, walker, WalkDirection::DOWN, 0);

        LeafDispatcher::dispatch(node, walker, WalkCmd::LAST_LEAF, 0, result.idx());

        i->leaf() = node;

        walker.finish(*i.get(), result.idx(), WalkCmd::LAST_LEAF);
    }

    return i;
}




#undef M_TYPE
#undef M_PARAMS

}

#endif
