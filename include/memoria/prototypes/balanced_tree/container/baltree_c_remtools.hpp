
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

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                         	Position;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int  Indexes                                                   = Base::Indexes;

    typedef std::function<void (const TreePath&, const TreePath&, Int)>			MergeFn;


    /**
     * \brief Try to merge tree node with its siblings at the specified level.
     *
     * First it tries to merge with left sibling, and if it fails then with the right one.
     * The *path* object is valid after merge.
     *
     * \param path path to the node
     * \param level level at the tree of the node
     *
     * \return true if node has been merged
     *
     * \see {mergeWithLeftSibling, mergeWithRightSibling} for details
     */

    MergeType mergeWithSiblings(TreePath& path, Int level)
    {
        Position idx(0);
        return self().mergeWithSiblings(path, level, idx);
    }



    bool mergeWithLeftSibling(TreePath& path, Int level, MergeFn fn = [](const TreePath&, const TreePath&, Int){});
    bool mergeWithRightSibling(TreePath& path, Int level);
    MergeType mergeWithSiblings(TreePath& path, Int level, MergeFn fn = [](const TreePath&, const TreePath&, Int){});

    bool mergePaths(TreePath& tgt, TreePath& src, Int level = 0);

    MEMORIA_DECLARE_NODE_FN_RTN(ShouldBeMergedNodeFn, shouldBeMergedWithSiblings, bool);
    bool shouldMergeNode(const TreePath& path, Int level) const
    {
    	const NodeBaseG& node = path[level].node();
    	return NodeDispatcher::dispatchConstRtn(node, ShouldBeMergedNodeFn());
    }












    Position removeRoom(
    		TreePath& path,
    		Int level,
    		const Position& from,
    		const Position& count,
    		Accumulator& accumulator,
    		bool remove_children = true
    );



    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------


    void removeRedundantRoot(TreePath& path, Int level);
    void removeRedundantRoot(TreePath& start, TreePath& stop, Int level);


    /**
     * \brief Remove a page from the btree. Do recursive removing if page's parent
     * has no more children.
     *
     * If after removing the parent is less than half filled than
     * merge it with siblings.
     */

    void removePage1(TreePath& path, Int stream, Int& leaf_entry_idx);


    /**
     * \brief Delete a node with it's children.
     */
    Position removeNode(NodeBaseG node);

    bool changeRootIfSingular(NodeBaseG& parent, NodeBaseG& node);

    /**
     * \brief Check if two nodes can be merged.
     *
     * \param tgt path to the node to be merged with
     * \param src path to the node to be merged
     * \param level level of the node in the tree
     * \return true if nodes can be merged according to the current policy
     */

    MEMORIA_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);

    bool canMerge(TreePath& tgt, TreePath& src, Int level)
    {
        return NodeDispatcher::dispatchConstRtn2(src[level].node(), tgt[level].node(), CanMergeFn());
    }

    /**
     * \brief Check if two nodes have the same parent.
     *
     *
     * \param left one path
     * \param right another path
     * \param level level of nodes in the paths
     * \return true if both nodes have the same parent
     */
    static bool isTheSameParent(TreePath& left, TreePath& right, Int level)
    {
        return left[level + 1].node() == right[level + 1].node();
    }

    void mergeNodes(TreePath& tgt, TreePath& src, Int level);
    bool mergeBTreeNodes(TreePath& tgt, TreePath& src, Int level, MergeFn fn);


    MEMORIA_DECLARE_NODE_FN_RTN(RemoveSpaceFn, removeSpace, Accumulator);
    MEMORIA_DECLARE_NODE_FN(MergeNodesFn, mergeWith);

    MEMORIA_PUBLIC void drop();

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::RemoveToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


/**
 * \brief Remove 'count' elements from tree node starting from 'from' element.
 *
 * \dotfile removeRoom.dot
 *
 * \param path   path to the node
 * \param level  level of the node at the path
 * \param from   start element
 * \param count  number of elements to remove
 *
 * \param accumulator       accumulator to put cumulative values of removed elements
 * \param remove_children   if true, remove all child nodes. FIXME: what is this?
 *
 */

M_PARAMS
typename M_TYPE::Position M_TYPE::removeRoom(
		TreePath& path,
		Int level,
		const Position& from,
		const Position& count,
		Accumulator& accumulator,
		bool remove_children
)
{
    //FIXME: optimize for the case when count == 0
	auto& self = this->self();

    Position key_count;

    if (count.gtAny(0))
    {
        NodeBaseG& node = path[level].node();
        node.update();

        Accumulator tmp_accum;

        if (remove_children)
        {
            if (!node->is_leaf())
            {
                Int stop = from.get() + count.get();
            	for (Int c = from.get(); c < stop; c++)
                {
                    NodeBaseG child = self.getChild(node, c, Allocator::READ);
                    Position tmp 	= self.removeNode(child);
                    key_count 		+= tmp;
                }
            }
        }

        VectorAdd(tmp_accum, NodeDispatcher::dispatchRtn(node, RemoveSpaceFn(), from, count));

        self.updateChildren(node, from.get());

        if (node->is_leaf()) {
        	key_count += count;
        }

        self.updateParentIfExists(path, level, -tmp_accum);

        if (level > 0)
        {
        	path.moveLeft(level - 1, from.get(), count.get());
        }

        VectorAdd(accumulator, tmp_accum);
    }

    return key_count;
}





/**
 * \brief Remove leaf node from tree.
 *
 * Remove the leaf node from tree. Do recursive removal up to the root if the node's parent
 * has no more children.
 *
 * If after removing the parent is less than half filled than
 * merge it with siblings.
 *
 *
 * \see removeNode
 */

M_PARAMS
void M_TYPE::removePage1(TreePath& path, Int stream, Int& leaf_entry_idx)
{
	auto& self = this->self();

	for (Int c = 1; c < path.getSize(); c++)
    {
		Int node_size = self.getNodeSize(path[c], 0);

		if (node_size > 1)
        {
            Accumulator accum;

            Int idx = path[c - 1].parent_idx();
            self.removeRoom(path, c, Position(idx), Position(1), accum);

            if (idx == node_size)
            {
                if (self.getNextNode(path, c, true))
                {
                    leaf_entry_idx = 0;
                }
                else
                {
                    for (Int d = c - 1; d >= 0; d--)
                    {
                        path[d].node()          = self.getLastChild(path[d + 1].node(), Allocator::READ);
                        path[d].parent_idx()    = self.getNodeSize(path[d + 1], 0) - 1;
                    }

                    leaf_entry_idx = self.getNodeSize(path.leaf(), stream);
                }
            }
            else
            {
                for (Int d = c - 1; d >= 0; d--)
                {
                    path[d].node()          = self.getChild(path[d + 1].node(), idx, Allocator::READ);
                    path[d].parent_idx()    = idx;

                    idx = 0;
                }

                leaf_entry_idx = 0;
            }

            return;
        }
        else if (path[c]->is_root())
        {
        	self.removeNode(path[c].node());

            NodeBaseG node = self.createRootNode1(0, true, me()->getRootMetadata());

            self.set_root(node->id());
            path.clear();
            path.append(TreePathItem(node, 0));

            leaf_entry_idx = 0;

            return;
        }
    }

	self.removeNode(path.leaf().node());

    NodeBaseG node = self.createRootNode1(0, true, self.getRootMetadata());

    self.set_root(node->id());
    path.clear();
    path.append(TreePathItem(node, 0));

    leaf_entry_idx = 0;
}

/**
 * \brief Remove node with all its children from the tree.
 *
 * \return number of children have been removed.
 *
 * \see removePage
 */

M_PARAMS
typename M_TYPE::Position M_TYPE::removeNode(NodeBaseG node)
{
    auto& self = this->self();

	const Int children_count = self.getNodeSize(node, 0);

    Position count;

    if (!node->is_leaf())
    {
        for (Int c = 0; c < children_count; c++)
        {
            NodeBaseG child = self.getChild(node, c, Allocator::READ);
            count += self.removeNode(child);
        }
    }
    else {
    	Position tmp = self.getNodeSizes(node);
    	count += tmp;
    }

    if (node->is_root())
    {
        ID id;
        id.setNull();
        self.set_root(id);
    }

    self.allocator().removePage(node->id());

    return count;
}


/**
 * If *parent* is a root and has only one child then the child is converted to root and returned in *node*.
 * Old root node is removed.
 *
 * \return true if *node* is now a root.
 */

M_PARAMS
bool M_TYPE::changeRootIfSingular(NodeBaseG& parent, NodeBaseG& node)
{
	auto& self = this->self();

	if (parent.isSet() && parent->is_root() && self.getNodeSize(parent, 0) == 1)
    {
        Metadata meta = self.getRootMetadata();

        self.node2Root(node, meta);

        self.set_root(node->id());

        self.allocator().removePage(parent->id());

        return true;
    }
    else {
        return false;
    }
}




/**
 * \brief Removes singular node chain starting from the tree root down to the specified level.
 *
 * Singular node chain is a chain of tree nodes where each node (except leaf)
 * has exactly one child. See the picture below for details.
 *
 * \dotfile removeRedundantRoot.dot

 * This call iteratively removes nodes in singular node chain starting from the root down to the *level*.
 */

M_PARAMS
void M_TYPE::removeRedundantRoot(TreePath& path, Int level)
{
	auto& self = this->self();

    for (Int c = path.getSize() - 1; c > level; c--)
    {
        NodeBaseG& node = path[c].node();

        if (self.getNodeSize(node, 0) == 1)
        {
            Metadata root_metadata = self.getRootMetadata();

            NodeBaseG& child = path[c - 1].node();

            if (self.canConvertToRoot(child))
            {
            	self.node2Root(child, root_metadata);

            	self.allocator().removePage(node->id());

            	self.set_root(child->id());

            	path.removeLast();
            }
            else {
            	break;
            }
        }
        else {
            break;
        }
    }
}
/**
 * \brief Removes singular node chain starting from the tree root down to the specified level.
 *
 * This call is intended to be used in batch removal operations where we have two iterators: the *start*
 * one and the *stop* one. After internal nodes are removed and range nodes are merged, we might get a redundant root
 * as a result. See the picture below for details.
 *
 * This call iteratively removes nodes in singular node chain starting from the root down to the *level*. It gets two
 * paths as a parameters. Both ones must contains the same nodes at least down to the *level*. After the call is
 * completed both paths are started from the new root node.
 *
 * \see mergeBTreeNodes, mergePaths
 */

M_PARAMS
void M_TYPE::removeRedundantRoot(TreePath& first, TreePath& second, Int level)
{
    auto& self = this->self();

	for (Int c = first.getSize() - 1; c > level; c--)
    {
        NodeBaseG& node = first[c].node();

        if (self.getNodeSize(node, 0) == 1)
        {
            Metadata root_metadata = self.getRootMetadata();

            NodeBaseG& child = first[c - 1].node();

            if (self.canConvertToRoot(child))
            {
            	self.node2Root(child, root_metadata);

            	self.allocator().removePage(node->id());

            	self.set_root(child->id());

                first.removeLast();

                second.removeLast();
            }
            else {
                break;
            }
        }
        else {
            break;
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
MergeType M_TYPE::mergeWithSiblings(TreePath& path, Int level, MergeFn fn)
{
	auto& self = this->self();

    if (self.mergeWithRightSibling(path, level))
    {
        return MergeType::RIGHT;
    }
    else if (self.mergeWithLeftSibling(path, level, fn))
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
bool M_TYPE::mergeWithLeftSibling(TreePath& path, Int level, MergeFn fn)
{
	auto& self = this->self();

    bool merged = false;

    if (self.shouldMergeNode(path, level))
    {
        TreePath prev = path;

        if (self.getPrevNode(prev, level))
        {
            merged = mergeBTreeNodes(prev, path, level, fn);

            if (merged)
            {
                path = prev;
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
bool M_TYPE::mergeWithRightSibling(TreePath& path, Int level)
{
    bool merged = false;

    auto& self = this->self();

    if (self.shouldMergeNode(path, level))
    {
        TreePath next = path;

        if (self.getNextNode(next))
        {
            merged = mergeBTreeNodes(path, next, level, [](const TreePath&, const TreePath&, Int){});
        }
    }

    return merged;
}

/**
 * \brief Merge *src* path to the *tgt* path from the tree root down to the specified *level*.
 *
 * If after nodes have been merged the resulting path is redundant, that means it consists from a single node chain,
 * then this path is truncated from the tree root down to the specified *level*.
 *
 * Unlike this call, \ref mergeBTreeNodes does not try to merge parents if nodes at the specified *level* can't be merged.
 *
 * \param tgt path to node to be merged with
 * \param src path to node to be merged
 * \param level level of the node in the tree
 * \return true if paths at the specified *level* was merged
 *
 * \see canMerge, removeRedundantRoot
 * \see mergeBTreeNodes
 */

M_PARAMS
bool M_TYPE::mergePaths(TreePath& tgt, TreePath& src, Int level)
{
    bool merged_at_level = false;

    for (Int c = tgt.getSize() - 1; c >= level; c--)
    {
        if (canMerge(tgt, src, c))
        {
            if (tgt[c] != src[c])
            {
                mergeNodes(tgt, src, c);

                src[c] = tgt[c];

                merged_at_level = c == level;
            }
        }
        else {
            break;
        }
    }

    removeRedundantRoot(tgt, src, level);

    return merged_at_level;
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
void M_TYPE::mergeNodes(TreePath& tgt, TreePath& src, Int level)
{
	auto& self = this->self();

    NodeBaseG& tgt_page = tgt[level].node();
    NodeBaseG& src_page = src[level].node();

    tgt_page.update();

    Int tgt_children_count = self.getNodeSize(tgt_page, 0);

    Int tgt_size = self.getNodeSize(tgt_page, 0);

    NodeDispatcher::dispatch2(src_page, tgt_page, MergeNodesFn());

    self.updateChildren(tgt_page, tgt_size);

    NodeBaseG& parent   = src[level + 1].node();
    Int parent_idx      = src[level].parent_idx();

    Accumulator accum;

    removeRoom(src, level + 1, Position(parent_idx), Position(1), accum, false);

    self.updateUp(src, level + 1, parent_idx - 1, accum, [](Int, Int){});

    self.allocator().removePage(src_page->id());

    src[level] = tgt[level];

    if (level > 0)
    {
    	src.moveRight(level - 1, 0, tgt_children_count);
    }

    self.reindex(parent); //FIXME: is it necessary?
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
bool M_TYPE::mergeBTreeNodes(TreePath& tgt, TreePath& src, Int level, MergeFn fn)
{
    if (canMerge(tgt, src, level))
    {
        if (isTheSameParent(tgt, src, level))
        {
            fn(tgt, src, level);

        	mergeNodes(tgt, src, level);

            removeRedundantRoot(tgt, src, level);

            return true;
        }
        else
        {
            if (mergeBTreeNodes(tgt, src, level + 1, fn))
            {
            	fn(tgt, src, level);

                mergeNodes(tgt, src, level);

                removeRedundantRoot(tgt, src, level);

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
