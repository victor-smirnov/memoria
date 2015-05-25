
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

    typedef TypesType                                                           Types;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Position                                            Position;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    static const Int MAIN_STREAM = 0;

private:


public:

    template <typename Walker>
    Iterator find0(Int stream, Walker&& walker);

    template <typename Walker>
    Iterator find2(Walker&& walker);


    template <typename LeafPath>
    Iterator _find2GT(Int index, TargetType<LeafPath> key)
    {
    	typename Types::template FindGTForwardWalker<Types, LeafPath> walker(index, key);
    	return self().find2(walker);
    }

    template <typename LeafPath>
    Iterator _find2GE(Int index, TargetType<LeafPath> key)
    {
    	typename Types::template FindGEForwardWalker<Types, LeafPath> walker(index, key);
    	return self().find2(walker);
    }

    template <typename LeafPath>
    Iterator _rank(Int index, CtrSizeT pos)
    {
    	typename Types::template RankForwardWalker<Types, LeafPath> walker(index, pos);
    	return self().find2(walker);
    }

    template <typename LeafPath>
    Iterator _select(Int index, CtrSizeT rank)
    {
    	typename Types::template SelectForwardWalker<Types, LeafPath> walker(index, rank);
    	return self().find2(walker);
    }


    template <typename Walker>
    Int findFw(NodeBaseG& node, Int stream, Int idx, Walker&& walker);

    template <typename Walker>
    Int findBw(NodeBaseG& node, Int stream, Int idx, Walker&& walker);

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
    	void processChain(Walker&& walker, Int leaf_cnt = 0)
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
    		}
    		else {
    			BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::PREFIXES, start, end);
    		}
    	}
    };

    struct FindResult {
    	NodeBaseG node;
    	Int idx;
    	bool pass;

    	explicit FindResult(NodeBaseG _node, Int _idx, bool _pass = true): node(_node), idx(_idx), pass(_pass) {}
    };

    template <typename Walker>
    StreamOpResult findFw2(NodeBaseG& node, Int stream, Int idx, Walker&& walker);

    template <typename Walker>
    FindResult findFw2(NodeChain node_chain, Walker&& walker, WalkDirection direction = WalkDirection::UP);


    template <typename Walker>
    FindResult findBw2(NodeChain node_chain, Walker&& walker, WalkDirection direction = WalkDirection::UP);

    template <Int Stream>
    Iterator _seek(CtrSizeT position)
    {
    	typename Types::template SkipForwardWalker<Types, IntList<Stream>> walker(position);
    	return self().find2(walker);
    }


    MEMORIA_DECLARE_NODE_FN_RTN(SizesFn, size_sums, Position);
    MEMORIA_PUBLIC Position sizes() const
    {
    	NodeBaseG node = self().getRoot();
    	return NodeDispatcher::dispatch(node, SizesFn());
    }

    MEMORIA_PUBLIC Iterator streamBegin(Int stream)
    {
        return self().template _seek<0>(0);
    }

    MEMORIA_PUBLIC Iterator Begin()
    {
        return streamBegin(MAIN_STREAM);
    }

    MEMORIA_PUBLIC Iterator begin()
    {
        return streamBegin(MAIN_STREAM);
    }

    MEMORIA_PUBLIC Iterator streamRBegin(Int stream)
    {
        typename Types::template FindRBeginWalker<Types> walker(stream, self());
        return self().find0(stream, walker);
    }

    MEMORIA_PUBLIC Iterator RBegin()
    {
        return streamRBegin(MAIN_STREAM);
    }

    MEMORIA_PUBLIC Iterator streamEnd(Int stream)
    {
        typename Types::template FindEndWalker<Types> walker(stream, self());
        return self().find0(stream, walker);
    }

    MEMORIA_PUBLIC Iterator End()
    {
        return streamEnd(MAIN_STREAM);
    }


    MEMORIA_PUBLIC Iterator end()
    {
        Iterator iter(*me());
        iter.type() = Iterator::END;
        return iter;
    }

    MEMORIA_PUBLIC IterEndMark endm()
    {
        return IterEndMark();
    }

    MEMORIA_PUBLIC Iterator streamREnd(Int stream)
    {
        typename Types::template FindREndWalker<Types> walker(stream, self());
        return self().find0(stream, walker);
    }

    MEMORIA_PUBLIC Iterator REnd()
    {
        return streamREnd(MAIN_STREAM);
    }

    Iterator findGT(Int stream, BigInt key, Int key_num)
    {
        typename Types::template FindGTWalker<Types, IntList<0>> walker(stream, key_num - 1, key);

        return self().find0(walker);
    }

    Iterator findGE(Int stream, BigInt key, Int key_num)
    {
        typename Types::template FindGEWalker<Types, IntList<0>> walker(stream, key_num - 1, key);

        return self().find0(stream, walker);
    }

    template <typename Walker>
    void walkUp(NodeBaseG node, Int idx, Walker&& walker) const
    {
        NodeDispatcher::dispatch(node, walker, idx);

        while (!node->is_root())
        {
            idx = node->parent_idx();
            node = self().getNodeParent(node);

            NodeDispatcher::dispatch(node, walker, idx);
        }
    }

    template <typename Walker>
    void walkUp2(NodeBaseG node, Int idx, Walker&& walker) const
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
typename M_TYPE::Iterator M_TYPE::find0(Int stream, Walker&& walker)
{
    auto& self = this->self();
    walker.direction()  = WalkDirection::DOWN;

    NodeBaseG node = self.getRoot();
    if (node.isSet())
    {
        Iterator i(self);

        i.stream() = stream;

        Int size = self.getNodeSize(node, stream);

        if (size > 0)
        {
            bool out_of_range = false;

            while (!node->is_leaf())
            {
                Int idx;
                if (!out_of_range)
                {
                    idx = NodeDispatcher::dispatch(node, walker, 0);

                    size = self.getNodeSize(node, stream);

                    if (idx >= size)
                    {
                        out_of_range = true;
                        idx = size - 1;
                    }
                }
                else {
                    idx = self.getNodeSize(node, stream) - 1;
                }

                node = self.getChild(node, idx);
            }

            Int idx;
            if (!out_of_range)
            {
                i.idx() = idx = NodeDispatcher::dispatch(node, walker, 0);
            }
            else {
                i.idx() = idx = self.getNodeSize(node, stream);
            }

            i.leaf() = node;

            walker.finish(i, idx);
        }
        else {
            i.leaf() = node;

            walker.empty(i);
        }

        i.init();
        return i;
    }
    else {
        return Iterator(self);
    }
}



M_PARAMS
template <typename Walker>
Int M_TYPE::findFw(NodeBaseG& node, Int stream, Int start, Walker&& walker)
{
    auto& self = this->self();

    if (node->is_root())
    {
        walker.direction()  = WalkDirection::DOWN;
    }
    else {
        walker.direction()  = WalkDirection::UP;
    }

    Int size = self.getNodeSize(node, stream);

    Int idx;

    if (start < size)
    {
        idx = NodeDispatcher::dispatch(node, walker, start);
    }
    else {
        idx = size;
    }

    if (idx >= size)
    {
        if (!node->is_root())
        {
            NodeBaseG parent = self.getNodeParent(node);

            // Step up the tree
            Int child_idx = findFw(parent, stream, node->parent_idx() + 1, walker);

            Int parent_size = self.getNodeSize(parent, stream);
            if (child_idx < parent_size)
            {
                // Step down the tree
                node = self.getChild(parent, child_idx);

                return NodeDispatcher::dispatch(node, walker, 0);
            }
            else {
                // Step down the tree
                node = self.getChild(parent, parent_size - 1);

                return self.getNodeSize(node, stream);
            }
        }
        else {
            walker.direction()  = WalkDirection::DOWN;
            return size;
        }
    }
    else {
        walker.direction()  = WalkDirection::DOWN;
        return idx;
    }
}





M_PARAMS
template <typename Walker>
Int M_TYPE::findBw(NodeBaseG& node, Int stream, Int start, Walker&& walker)
{
    auto& self = this->self();

    if (node->is_root())
    {
        walker.direction()  = WalkDirection::DOWN;
    }
    else {
        walker.direction()  = WalkDirection::UP;
    }

    Int idx;

    if (start >= 0)
    {
        idx = NodeDispatcher::dispatch(node, walker, start);
    }
    else {
        idx = -1;
    }

    if (idx < 0)
    {
        if (!node->is_root())
        {
            NodeBaseG parent = self.getNodeParent(node);

            // Step up the tree
            Int child_idx = findBw(parent, stream, node->parent_idx() - 1, walker);

            if (child_idx >= 0)
            {
                // Step down the tree
                node        = self.getChild(parent, child_idx);
                Int start   = self.getNodeSize(node, stream) - !node->is_leaf();

                return NodeDispatcher::dispatch(node, walker, start);
            }
            else {
                // Step down the tree
                node = self.getChild(parent, 0);

                return -1;
            }
        }
        else {
            return -1;
        }
    }
    else {
        walker.direction()  = WalkDirection::DOWN;
        return idx;
    }
}




/********************************************************************************************/

M_PARAMS
template <typename Walker>
typename M_TYPE::FindResult M_TYPE::findFw2(NodeChain node_chain, Walker&& walker, WalkDirection direction)
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
    			node_chain.processChain(std::forward<Walker>(walker));
    			return FindResult(node, result.idx());
    		}
    		else {
    			auto child = self.getChild(node, result.idx());
    			return findFw2(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    		}
    	}
    	else {
    		if (!node_chain.node->is_root())
    		{
    			auto parent 		= self.getNodeParent(node);
    			auto parent_idx 	= node->parent_idx() + 1;
    			auto parent_result  = findFw2(NodeChain(parent, parent_idx, &node_chain), std::forward<Walker>(walker), WalkDirection::UP);

    			if (parent_result.pass)
    			{
    				return parent_result;
    			}
    		}

    		if (node->is_leaf())
    		{
    			node_chain.processChain(std::forward<Walker>(walker));
    			return FindResult(node, result.idx());
    		}
    		else if (!result.empty())
    		{
    			BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.idx() - 1);
    			node_chain.end = result.idx() - 1;

    			auto child = self.getChild(node, result.idx() - 1);
    			return findFw2(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    		}
    		else {
    			return FindResult(node, start, false);
    		}
    	}
    }
    else if (node_chain.node->is_leaf())
    {
    	node_chain.processChain(std::forward<Walker>(walker));
    	return FindResult(node_chain.node, result.idx());
    }
    else if (!result.out_of_range())
    {
    	auto child = self.getChild(node_chain.node, result.idx());
    	return findFw2(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }
    else
    {
    	BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.idx() - 1);
    	node_chain.end = result.idx() - 1;

    	auto child = self.getChild(node_chain.node, result.idx() - 1);
    	return findFw2(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }

}




M_PARAMS
template <typename Walker>
typename M_TYPE::FindResult M_TYPE::findBw2(NodeChain node_chain, Walker&& walker, WalkDirection direction)
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
    			node_chain.processChain(std::forward<Walker>(walker));
    			return FindResult(node_chain.node, result.idx());
    		}
    		else {
    			auto child = self.getChild(node_chain.node, result.idx());
    			return findBw2(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    		}
    	}
    	else {
    		if (!node_chain.node->is_root())
    		{
    			auto parent 		= self.getNodeParent(node_chain.node);
    			auto parent_idx 	= node_chain.node->parent_idx() - 1;
    			auto parent_result  = findBw2(NodeChain(parent, parent_idx, &node_chain), std::forward<Walker>(walker), WalkDirection::UP);

    			if (parent_result.pass)
    			{
    				return parent_result;
    			}
    		}

    		if (node_chain.node->is_leaf())
    		{
    			node_chain.processChain(std::forward<Walker>(walker));
    			return FindResult(node_chain.node, result.idx());
    		}
    		else if (!result.empty())
    		{
    			BranchDispatcher::dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.idx());
    			node_chain.end = result.idx();

    			auto child = self.getChild(node_chain.node, result.idx() + 1);
    			return findBw2(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    		}
    		else {
    			return FindResult(node_chain.node, node_chain.start, false);
    		}
    	}
    }
    else if (node_chain.node->is_leaf())
    {
    	node_chain.processChain(std::forward<Walker>(walker));
    	return FindResult(node_chain.node, result.idx());
    }
    else if (!result.out_of_range())
    {
    	auto child = self.getChild(node_chain.node, result.idx());
    	return findBw2(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }
    else
    {
    	BranchDispatcher::dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.idx());
    	node_chain.end = result.idx();

    	auto child = self.getChild(node_chain.node, result.idx() + 1);
    	return findBw2(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }

}





M_PARAMS
template <typename Walker>
typename M_TYPE::Iterator M_TYPE::find2(Walker&& walker)
{
    auto& self = this->self();

    NodeBaseG node = self.getRoot();
    if (node.isSet())
    {
        Iterator i(self);

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

        i.leaf() = node;

        walker.finish(i, result.idx());

        return i;
    }
    else {
        return Iterator(self);
    }
}




#undef M_TYPE
#undef M_PARAMS

}

#endif
