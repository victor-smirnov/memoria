
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMTOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMTOOLS_HPP

#include <memoria/core/container/macros.hpp>
#include <memoria/prototypes/balanced_tree/baltree_types.hpp>
#include <functional>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::RemoveToolsName)

	typedef TypesType															Types;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::ID                                                   ID;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                         	Position;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int  Indexes                                                   = Base::Indexes;

    typedef std::function<void (const NodeBaseG&, const NodeBaseG&)>			MergeFn;

    void removeNode(NodeBaseG& node, Accumulator& accum, Position& sizes);

    MEMORIA_DECLARE_NODE_FN(RemoveNodeContentFn, removeSpace);
    void removeNodeContent(NodeBaseG& node, Int start, Int end, Accumulator& accum, Position& sizes);

    MEMORIA_DECLARE_NODE_FN_RTN(RemoveLeafContentFn, removeSpace, Accumulator);
    Accumulator removeLeafContent(NodeBaseG& node, const Position& start, const Position& end);

    Accumulator removeLeafContent(NodeBaseG& node, Int stream, Int start, Int end);


    MEMORIA_DECLARE_NODE_FN_RTN(RemoveNonLeafNodeEntryFn, removeSpaceAcc, Accumulator);
    void removeNonLeafNodeEntry(NodeBaseG& node, Int idx);





    bool mergeWithLeftSibling(NodeBaseG& node, MergeFn fn = [](const NodeBaseG&, const NodeBaseG&){});
    bool mergeWithRightSibling(NodeBaseG& node);
    MergeType mergeWithSiblings(NodeBaseG& node, MergeFn fn = [](const NodeBaseG&, const NodeBaseG&){});


    MEMORIA_DECLARE_NODE_FN_RTN(ShouldBeMergedNodeFn, shouldBeMergedWithSiblings, bool);
    bool shouldMergeNode(const NodeBaseG& node) const
    {
    	return NodeDispatcher::dispatchConstRtn(node, ShouldBeMergedNodeFn());
    }




    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------



    void removeRedundantRootP(NodeBaseG& node);



    /**
     * \brief Check if two nodes can be merged.
     *
     * \param tgt path to the node to be merged with
     * \param src path to the node to be merged
     * \param level level of the node in the tree
     * \return true if nodes can be merged according to the current policy
     */

    MEMORIA_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);

    bool canMerge(const NodeBaseG& tgt, const NodeBaseG& src)
    {
        return NodeDispatcher::dispatchConstRtn2(src, tgt, CanMergeFn());
    }



    bool isTheSameParent(const NodeBaseG& left, const NodeBaseG& right)
    {
    	return left->parent_id() == right->parent_id();
    }

    void mergeNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeBTreeNodes(NodeBaseG& tgt, NodeBaseG& src, MergeFn fn = [](const NodeBaseG&, const NodeBaseG&){});


    MEMORIA_DECLARE_NODE_FN_RTN(RemoveSpaceFn, removeSpace, Accumulator);
    MEMORIA_DECLARE_NODE_FN(MergeNodesFn, mergeWith);

    MEMORIA_PUBLIC void drop();

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::RemoveToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::removeNode(NodeBaseG& node, Accumulator& sums, Position& sizes)
{
	auto& self = this->self();

	if (!node->is_leaf())
	{
		Int size = self.getNodeSize(node, 0);
		self.forAllIDs(node, 0, size, [&, this](const ID& id, Int idx){
			NodeBaseG child = self.allocator().getPage(id, Allocator::READ);
			this->removeNode(child, sums, sizes);
			self.allocator().removePage(id);
		});
	}
	else {
		VectorAdd(sums, self.getLeafSums(node));
		sizes += self.getNodeSizes(node);
	}

	self.allocator().removePage(node->id());
}

M_PARAMS
void M_TYPE::removeNodeContent(NodeBaseG& node, Int start, Int end, Accumulator& sums, Position& sizes)
{
	auto& self = this->self();

	MEMORIA_ASSERT_TRUE(!node->is_leaf());

	self.forAllIDs(node, start, end, [&, this](const ID& id, Int idx){
		NodeBaseG child = self.allocator().getPage(id, Allocator::READ);
		self.removeNode(child, sums, sizes);
	});

	NonLeafDispatcher::dispatch(node, RemoveNodeContentFn(), start, end);
}


M_PARAMS
void M_TYPE::removeNonLeafNodeEntry(NodeBaseG& node, Int start)
{
	auto& self = this->self();

	MEMORIA_ASSERT_TRUE(!node->is_leaf());

	node.update();
	Accumulator sums = NonLeafDispatcher::dispatchRtn(node, RemoveNonLeafNodeEntryFn(), start, start + 1);

	if (!node->is_root())
	{
		NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);

		self.updatePath(parent, node->parent_idx(), sums);
	}
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeLeafContent(NodeBaseG& node, const Position& start, const Position& end)
{
	auto& self = this->self();

	node.update();

	Accumulator sums = LeafDispatcher::dispatchRtn(node, RemoveLeafContentFn(), start, end);

	if (!node->is_root())
	{
		NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);

		self.updatePath(parent, node->parent_idx(), -sums);
	}

	return sums;
}

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeLeafContent(NodeBaseG& node, Int stream, Int start, Int end)
{
	auto& self = this->self();

	node.update();
	Accumulator sums = LeafDispatcher::dispatchRtn(node, RemoveLeafContentFn(), stream, start, end);

	if (!node->is_root())
	{
		NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);

		self.updatePath(parent, node->parent_idx(), -sums);
	}

	return sums;
}



M_PARAMS
void M_TYPE::removeRedundantRootP(NodeBaseG& node)
{
	auto& self = this->self();

    if (!node->is_root())
    {
    	NodeBaseG parent = self.getNodeParent(node, Allocator::READ);
    	if (!parent->is_root())
    	{
    		removeRedundantRootP(parent);
    	}

    	if (parent->is_root())
    	{
    		Int size = self.getNodeSize(node, 0);
    		if (size == 1)
    		{
    			Metadata root_metadata = self.getRootMetadata();

    			// FIXME redesigne it to use tryConvertToRoot(node) instead
    			if (self.canConvertToRoot(node))
    			{
    				self.node2Root(node, root_metadata);

    				self.allocator().removePage(parent->id());

    				self.set_root(node->id());
    			}
    		}
    	}
    }
}




/**
 * \brief Merge node with its siblings (if present).
 *
 * First try to merge with right sibling, then with left sibling.
 *
 * \param path path to the node
 * \param level level at the tree of the node
 * \param key_idx some key index in the merging node. After merge the value will be incremented with the size of
 * the merged sibling.
 * \return true if the node have been merged
 *
 * \see mergeWithRightSibling, mergeWithLeftSibling
 */

M_PARAMS
MergeType M_TYPE::mergeWithSiblings(NodeBaseG& node, MergeFn fn)
{
	auto& self = this->self();

    if (self.mergeWithRightSibling(node))
    {
        return MergeType::RIGHT;
    }
    else if (self.mergeWithLeftSibling(node, fn))
    {
        return MergeType::LEFT;
    }
    else {
    	return MergeType::NONE;
    }
}


/**
 * \brief Try to merge node with its left sibling (if present).
 *
 * Calls \ref shouldMergeNode to check if requested node should be merged with its left sibling, then merge if true.
 *
 * \param path path to the node
 * \param level level at the tree of the node
 * \param key_idx some key index in the merging node. After merge the value will be incremented with the
 * size of the merged sibling.
 *
 * \return true if node has been merged
 *
 * \see mergeWithRightSibling, shouldMergeNode for details
 */


M_PARAMS
bool M_TYPE::mergeWithLeftSibling(NodeBaseG& node, MergeFn fn)
{
	auto& self = this->self();

    bool merged = false;

    if (self.shouldMergeNode(node))
    {
        auto prev = self.getPrevNodeP(node);

        if (prev)
        {
            merged = mergeBTreeNodes(prev, node, fn);

            if (merged)
            {
                node = prev;
            }
        }
        else {
            merged = false;
        }
    }

    return merged;
}

/**
 * \brief Merge node with its right sibling (if present)
 *
 * Calls \ref shouldMergeNode to check if requested node should be merged with its right sibling, then merge if true.
 *
 * \param path path to the node
 * \param level level of the node in the tree

 * \return true if node has been merged
 *
 * \see mergeWithLeftSibling, shouldMergeNode for details
 */

M_PARAMS
bool M_TYPE::mergeWithRightSibling(NodeBaseG& node)
{
    bool merged = false;

    auto& self = this->self();

    if (self.shouldMergeNode(node))
    {
        auto next = self.getNextNodeP(node);

        if (next)
        {
            merged = mergeBTreeNodes(node, next, [](const NodeBaseG&, const NodeBaseG&){});
        }
    }

    return merged;
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

    Int tgt_size = self.getNodeSize(tgt, 0);

    NodeDispatcher::dispatch2(src, tgt, MergeNodesFn());

    self.updateChildren(tgt, tgt_size);

    NodeBaseG src_parent   	= self.getNodeParent(src, Allocator::READ);
    Int parent_idx      	= src->parent_idx();

    MEMORIA_ASSERT(parent_idx, >, 0);

    Accumulator sums 		= self.getNonLeafKeys(src_parent, parent_idx);

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

    if (canMerge(tgt, src))
    {
        if (isTheSameParent(tgt, src))
        {
            fn(tgt, src);

        	mergeNodes(tgt, src);

            removeRedundantRootP(tgt);

            return true;
        }
        else
        {
            NodeBaseG tgt_parent = self.getNodeParent(tgt, Allocator::READ);
            NodeBaseG src_parent = self.getNodeParent(src, Allocator::READ);

        	if (mergeBTreeNodes(tgt_parent, src_parent, fn))
            {
            	fn(tgt, src);

                mergeNodes(tgt, src);

                removeRedundantRootP(tgt);

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


/**
 * \brief Delete container from allocator with all associated data.
 *
 * Note that this call does not destruct container object.
 */
MEMORIA_PUBLIC M_PARAMS
void M_TYPE::drop()
{
//	 FIXME: remove record from allocator's root directory.

    NodeBaseG root = self().getRoot(Allocator::READ);

    if (root.isSet())
    {
    	self().removeNode(root);
    }
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
