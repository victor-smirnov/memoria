
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_NODENORM_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_NODENORM_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::NodeNormName)

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


    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position 											Position;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    typedef typename Types::PageUpdateMgr										PageUpdateMgr;

    typedef std::function<Accumulator (NodeBaseG&, NodeBaseG&)> 				SplitFn;

    static const Int Indexes                                                    = Types::Indexes;
    static const Int Streams                                                    = Types::Streams;

    typedef std::function<void (const Position&, Int)>							MergeFn;

    void insertNonLeafP(NodeBaseG& node, Int idx, const Accumulator& keys, const ID& id);

    NodeBaseG splitPathP(NodeBaseG& node, Int split_at);
    NodeBaseG splitLeafP(NodeBaseG& leaf, const Position& split_at);

    NodeBaseG splitP(NodeBaseG& node, SplitFn split_fn);

    MEMORIA_DECLARE_NODE_FN(UpdateNodeFn, updateUp);
    bool updateNode(NodeBaseG& node, Int idx, const Accumulator& keys);
    void updatePath(NodeBaseG& node, Int& idx, const Accumulator& keys);
    void updateParent(NodeBaseG& node, const Accumulator& sums);





    void updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, std::function<void (Int, Int)> fn);

    void updateParentIfExists(TreePath& path, Int level, const Accumulator& counters);


    MEMORIA_DECLARE_NODE_FN(InsertFn, insert);

    void insertNonLeaf(
    		TreePath& path,
    		Int level,
    		Int idx,
    		const Accumulator& keys,
    		const ID& id
    );

    void insertNonLeaf(
    		NodeBaseG& node,
    		Int idx,
    		const Accumulator& keys,
    		const ID& id
    );


    MEMORIA_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);
    bool canMerge(const NodeBaseG& tgt, const NodeBaseG& src)
    {
        return NodeDispatcher::dispatchConstRtn2(src, tgt, CanMergeFn());
    }


    MEMORIA_DECLARE_NODE_FN(MergeNodesFn, mergeWith);
    void mergeNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeBTreeNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const Position&, Int){});


MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::NodeNormName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::insertNonLeafP(NodeBaseG& node, Int idx, const Accumulator& keys, const ID& id)
{
	auto& self = this->self();

	node.update();
	NonLeafDispatcher::dispatch(node, InsertFn(), idx, keys, id);
	self.updateChildren(node, idx);

	if (!node->is_root())
	{
		NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);
		self.updatePath(parent, node->parent_idx(), keys);
	}
}



M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::splitP(NodeBaseG& left_node, SplitFn split_fn)
{
	auto& self = this->self();

	if (left_node->is_root())
	{
		self.newRootP(left_node);
	}

	left_node.update();
	NodeBaseG left_parent = self.getNodeParent(left_node, Allocator::UPDATE);

	NodeBaseG other  = self.createNode1(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

	Accumulator keys = split_fn(left_node, other);

	Int parent_idx   = left_node->parent_idx();

	self.updatePath(left_parent, parent_idx, -keys);

	if (self.getNonLeafCapacity(left_parent, -1) > 0)
	{
		self.insertNonLeafP(left_parent, parent_idx + 1, keys, other->id());
	}
	else {
		NodeBaseG right_parent = splitPathP(left_parent, parent_idx + 1);

		self.insertNonLeafP(right_parent, 0, keys, other->id());
	}

	return other;
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::splitPathP(NodeBaseG& left_node, Int split_at)
{
	auto& self = this->self();

	return splitP(left_node, [&self, split_at](NodeBaseG& left, NodeBaseG& right){
		return self.splitNonLeafNode(left, right, split_at);
	});
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::splitLeafP(NodeBaseG& left_node, const Position& split_at)
{
	auto& self = this->self();

	return splitP(left_node, [&](NodeBaseG& left, NodeBaseG& right){
		return self.splitLeafNode(left, right, split_at);
	});
}

M_PARAMS
bool M_TYPE::updateNode(NodeBaseG& node, Int idx, const Accumulator& keys)
{
	NonLeafDispatcher::dispatch(node, UpdateNodeFn(), idx, keys);
	return true;
}

M_PARAMS
void M_TYPE::updatePath(NodeBaseG& node, Int& idx, const Accumulator& keys)
{
	auto& self = this->self();

	NodeBaseG tmp = node;

	self.updateNode(tmp, idx, keys);

	while(!tmp->is_root())
	{
		Int parent_idx = tmp->parent_idx();
		tmp = self.getNodeParent(tmp, Allocator::UPDATE);

		self.updateNode(tmp, parent_idx, keys);
	}
}

M_PARAMS
void M_TYPE::updateParent(NodeBaseG& node, const Accumulator& sums)
{
	auto& self = this->self();

	if (!node->is_root())
	{
		NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);

		self.updatePath(parent, node->parent_idx(), sums);
	}
}









M_PARAMS
void M_TYPE::updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, std::function<void (Int, Int)> fn)
{
    auto& self = this->self();

	for (Int c = level; c < path.getSize(); c++)
    {
		if (self.updateCounters(path, c, idx, counters, fn))
        {
            break;
        }
        else {
            idx = path[c].parent_idx();
        }
    }
}


M_PARAMS
void M_TYPE::updateParentIfExists(TreePath& path, Int level, const Accumulator& counters)
{
	auto& self = this->self();

	if (level < path.getSize() - 1)
    {
        self.updateUp(path, level + 1, path[level].parent_idx(), counters, [](Int, Int){});
    }
}


M_PARAMS
void M_TYPE::insertNonLeaf(
		TreePath& path,
		Int level,
		Int idx,
		const Accumulator& keys,
		const ID& id
)
{
	auto& self = this->self();
	NodeBaseG& node = path[level];

	node.update();

	NonLeafDispatcher::dispatch(node, InsertFn(), idx, keys, id);
}


M_PARAMS
void M_TYPE::insertNonLeaf(
		NodeBaseG& node,
		Int idx,
		const Accumulator& keys,
		const ID& id
)
{
	node.update();
	NonLeafDispatcher::dispatch(node, InsertFn(), idx, keys, id);
}






/**
 * \brief Merge *src* path to the *tgt* path unconditionally.
 *
 * Perform merging of two paths, *src* to *dst* at the specified *level*. Both nodes (at boths paths) must
 * have the same parent.
 *
 * \param tgt path to node to be merged with
 * \param src path to node to be merged
 * \param level level of the node in the tree
 * \return true if paths have been merged
 *
 * \see mergeWithSiblings - this is the basic method
 */

M_PARAMS
void M_TYPE::mergeNodes(NodeBaseG& tgt, NodeBaseG& src)
{
	auto& self = this->self();

    tgt.update();
    src.update();

    Int tgt_size = self.getNodeSize(tgt, 0);

    NodeDispatcher::dispatch2(src, tgt, MergeNodesFn());

    self.updateChildren(tgt, tgt_size);

    NodeBaseG src_parent   	= self.getNodeParent(src, Allocator::READ);
    Int parent_idx      	= src->parent_idx();

    MEMORIA_ASSERT(parent_idx, >, 0);

    Accumulator sums 		= self.sums(src_parent, parent_idx, parent_idx + 1);

    self.removeNonLeafNodeEntry(src_parent, parent_idx);

    Int idx = parent_idx - 1;

    self.updatePath(src_parent, idx, sums);

    self.allocator().removePage(src->id());
}

/**
 * \brief Merge *src* path to the *tgt* path.
 *
 * Merge two tree paths, *src* to *dst* upward starting from nodes specified with *level*. If both these
 * nodes have different parents, then recursively merge parents first. Calls \ref canMerge to check if nodes can be merged.
 * This call will try to merge parents only if current nodes can be merged.
 *
 * If after nodes have been merged the resulting path is redundant, that means it consists from a single node chain,
 * then this path is truncated from the tree root down to the specified *level*.
 *
 * Unlike this call, \ref mergePaths tries to merge paths starting from the root down to the specified *level*.
 *
 * \param tgt path to node to be merged with
 * \param src path to node to be merged
 * \param level level of the node in the tree
 * \return true if paths have been merged
 *
 * \see mergeWithSiblings - this is the basic method
 * \see canMerge, removeRedundantRoot, mergeNodes, isTheSameParent
 */

M_PARAMS
bool M_TYPE::mergeBTreeNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn)
{
	auto& self = this->self();

    if (self.canMerge(tgt, src))
    {
        if (self.isTheSameParent(tgt, src))
        {
        	fn(self.getNodeSizes(tgt), tgt->level());

        	mergeNodes(tgt, src);

            self.removeRedundantRootP(tgt);

            return true;
        }
        else
        {
            NodeBaseG tgt_parent = self.getNodeParent(tgt, Allocator::READ);
            NodeBaseG src_parent = self.getNodeParent(src, Allocator::READ);

        	if (mergeBTreeNodes(tgt_parent, src_parent, fn))
            {
            	fn(self.getNodeSizes(tgt), tgt->level());

                mergeNodes(tgt, src);

                self.removeRedundantRootP(tgt);

                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
