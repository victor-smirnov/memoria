
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMOVE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMOVE_HPP

#include <memoria/core/container/macros.hpp>


namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::RemoveName)

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

    


    template <typename Node>
    void dataRemoveHandlerFn(Node* node, Int idx, Int count)
    {}

    MEMORIA_FN_WRAPPER(DataRemoveHandlerFn, dataRemoveHandlerFn);

    struct UpdateType {
        enum Enum {NONE, PARENT_ONLY, FULL};
    };




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

    bool mergeWithSiblings(TreePath& path, Int level)
    {
        Position idx(0);
        return self().mergeWithSiblings(path, level, idx);
    }



    bool mergeWithLeftSibling(TreePath& path, Int level, Position& key_idx);
    bool mergeWithRightSibling(TreePath& path, Int level);
    bool mergeWithSiblings(TreePath& path, Int level, Position& key_idx);

    bool mergePaths(TreePath& tgt, TreePath& src, Int level = 0);

    MEMORIA_DECLARE_NODE_FN_RTN(ShouldMergeNodeFn, shouldMergeWithSiblings, bool);
    bool shouldMergeNode(const TreePath& path, Int level) const
    {
    	const NodeBaseG& node = path[level].node();
    	return NodeDispatcher::dispatchConstRtn(node, ShouldMergeNodeFn());
    }





    BigInt removeEntries(
    		TreePath& from,
    		Position& from_idx,
    		TreePath& to,
    		Position& to_idx,
    		Accumulator& accum,
    		bool merge 					= true
    );

    MEMORIA_PUBLIC void drop();

    void removeAllPages(TreePath& start, TreePath& stop, Accumulator& accum, BigInt& removed_key_count);

    void removePagesFromStart(TreePath& stop, Position& stop_idx, Accumulator& accum, BigInt& removed_key_count);

    void removePagesAtEnd(TreePath& start, Position& start_idx, Accumulator& accum, BigInt& removed_key_count);

    void removePages(
            TreePath& start,
            Position& start_idx,
            TreePath& stop,
            Position& stop_idx,
            Int level,
            Accumulator& accum,
            BigInt& removed_key_count
    );



    BigInt removeRoom(
    		TreePath& path,
    		Int level,
    		const Position& from,
    		const Position& count,
    		Accumulator& accumulator,
    		bool remove_children = true
    );



    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------

    void removePagesInternal(
            TreePath& start,
            Position& start_idx,
            TreePath& stop,
            Position& stop_idx,
            Int level,
            Accumulator& accum,
            BigInt& removed_key_count
    );

    void removeRedundantRoot(TreePath& path, Int level);
    void removeRedundantRoot(TreePath& start, TreePath& stop, Int level);




    /**
     * \brief Remove a page from the btree. Do recursive removing if page's parent
     * has no more children.
     *
     * If after removing the parent is less than half filled than
     * merge it with siblings.
     */

    void removePage(TreePath& path, Int& idx);


    /**
     * \brief Delete a node with it's children.
     */
    BigInt removeNode(NodeBaseG node);

    bool changeRootIfSingular(NodeBaseG& parent, NodeBaseG& node);

    /**
     * \brief Check if two nodes can be merged.
     *
     * \param tgt path to the node to be merged with
     * \param src path to the node to be merged
     * \param level level of the node in the tree
     * \return true if nodes can be merged according to the current policy
     */

    MEMORIA_DECLARE_NODE2_FN_RTN(CanMergeFn, canMergeWith, bool);

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
    bool mergeBTreeNodes(TreePath& tgt, TreePath& src, Int level);


    MEMORIA_DECLARE_NODE_FN_RTN(RemoveSpaceFn, removeSpace, Accumulator);


    /**
     * Merge two nodes moving keys and values from 'page2' to 'page1' assuming
     * these two pages have the same parent.
     *
     */

//    template <typename Node1, typename Node2>
//    void mergeNodesFn(Node1 *page1, Node2 *page2)
//    {
//        page2->map().copyTo(&page1->map(), 0, page2->children_count(), page1->children_count());
//        page1->inc_size(page2->children_count());
//        page1->map().reindex();
//    }
//
//    MEMORIA_FN_WRAPPER(MergeNodesFn, mergeNodesFn);

    MEMORIA_DECLARE_NODE_FN(MergeNodesFn, mergeWith);


MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::RemoveName)
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
BigInt M_TYPE::removeRoom(
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

    BigInt key_count = 0;

    if (count > 0)
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
                    NodeBaseG child =   self.getChild(node, c, Allocator::READ);
                    Int tmp = self.removeNode(child);
                    key_count       +=  tmp;
                }
            }
        }

        VectorAdd(tmp_accum, NodeDispatcher::dispatchRtn(node, RemoveSpaceFn(), from, count));

        if (node->is_leaf()) {
        	key_count += count.get();
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
 * \brief Removes all entries from the tree starting with *from* iterator and ending with *to* one, but not include it.
 *
 * \param from
 * \param to
 * \param keys
 * \param merge if *true* then try to merge btree leafs if necessary.
 *
 * \return number of removed entries
 *
 * \see removeAllPages, removePagesFromStart, removePagesAtEnd, removePages
 * \see mergeWithRightSibling, addTotalKeyCount
 */

M_PARAMS
BigInt M_TYPE::removeEntries(
		TreePath& start,
		Position& start_idx,
		TreePath& stop,
		Position& stop_idx,
		Accumulator& keys,
		bool merge
)
{
	auto& self = this->self();

//	if (from.isEmpty() || from.isEnd())
//    {
//        return 0;
//    }
//
//    if (to.isEmpty())
//    {
//        return 0;
//    }

    BigInt removed_key_count = 0;

//    TreePath& start     = from.path();
//    Int&      start_idx = from.key_idx();
//
//    TreePath& stop      = to.path();
//    Int&      stop_idx  = to.key_idx();

    bool at_end = stop_idx.gteAll(self.getNodeSizes(stop.leaf()));

    bool from_start;

    if (start_idx.eqAll(0))
    {
        if (self.getPrevNode(start, 0))
        {
            start_idx = self.getNodeSizes(start.leaf());

            from_start = false;
        }
        else {
            from_start = true;
        }
    }
    else {
        from_start = false;
    }


    if (from_start && at_end)
    {
        removeAllPages(start, stop, keys, removed_key_count);

        start_idx       = stop_idx.setAll(0);
//        from.keyNum()   = to.keyNum()   = 0;
    }
    else if (from_start && !at_end)
    {
        removePagesFromStart(stop, stop_idx, keys, removed_key_count);

        if (merge) {
        	self.mergeWithRightSibling(stop, 0);
        }

//        to.keyNum() -= removed_key_count;

        start       = stop;
        start_idx   = stop_idx;
    }
    else if ((!from_start) && at_end)
    {
        removePagesAtEnd(start, start_idx, keys, removed_key_count);

        if (merge) {
        	self.mergeWithLeftSibling(start, 0, start_idx);
        }

        stop        = start;
        stop_idx    = start_idx;
    }
    else {
        removePages(start, start_idx, stop, stop_idx, 0, keys, removed_key_count);

        if (merge) {
        	self.mergeWithSiblings(stop, 0, stop_idx);
        }

//        to.keyNum() -= removed_key_count;

        start       = stop;
        start_idx   = stop_idx;
    }

    self.addTotalKeyCount(stop, -removed_key_count);

    return removed_key_count;
}




M_PARAMS
void M_TYPE::removeAllPages(TreePath& start, TreePath& stop, Accumulator& accum, BigInt& removed_key_count)
{
	auto& self = this->self();

    Int level = start.getSize() - 1;
    Position count = self.getNodeSizes(start[level].node());

    removed_key_count += removeRoom(start, level, Position(0), count, accum);


    self.removeNode(start[level].node());

    NodeBaseG node = self.createRootNode(0, true, me()->getRootMetadata());

    self.set_root(node->id());

    start.clear();
    stop.clear();

    start.append(TreePathItem(node, 0));
    stop. append(TreePathItem(node, 0));
}


M_PARAMS
void M_TYPE::removePagesFromStart(TreePath& stop, Position& stop_idx, Accumulator& accum, BigInt& removed_key_count)
{
    Position idx = stop_idx;

    for (Int c = 0; c < stop.getSize(); c++)
    {
        removed_key_count += removeRoom(stop, c, Position(0), idx, accum);
        idx = Position(stop[c].parent_idx());
    }

    stop_idx = Position(0);

    removeRedundantRoot(stop, 0);
}


M_PARAMS
void M_TYPE::removePagesAtEnd(TreePath& start, Position& start_idx, Accumulator& accum, BigInt& removed_key_count)
{
	auto& self = this->self();

	if (start_idx.eqAll(0))
    {
    	self.getPrevNode(start);
        start_idx = self.getNodeSizes(start.leaf());
    }

    Position idx = start_idx;

    for (Int c = 0; c < start.getSize(); c++)
    {
        if (idx > 0)
        {
            removed_key_count += removeRoom(start, c, idx, self.getNodeSizes(start[c].node()) - idx, accum);
            idx = Position(start[c].parent_idx() + 1);
        }
        else {
            idx = Position(start[c].parent_idx());
        }
    }

    removeRedundantRoot(start, 0);

//    self.finishPathStep(start, start_idx);
}


M_PARAMS
void M_TYPE::removePages(
                TreePath& start,
                Position& start_idx,
                TreePath& stop,
                Position& stop_idx,
                Int level,
                Accumulator& accum,
                BigInt& removed_key_count
)
{
    if (start_idx.eqAll(0))
    {
    	if (self().getPrevNode(start))
    	{
    		start_idx = self().getNodeSizes(start.leaf());
    	}
    }

    removePagesInternal(start, start_idx, stop, stop_idx, level, accum, removed_key_count);
}


M_PARAMS
void M_TYPE::removePagesInternal(
                TreePath& start,
                Position& start_idx,
                TreePath& stop,
                Position& stop_idx,
                Int level,
                Accumulator& accum,
                BigInt& removed_key_count)
{
	auto& self = this->self();

    if (self.isTheSameNode(start, stop, level))
    {
        // The root node of removed subtree

        if ((stop_idx - start_idx).gtAny(0))
        {
            //remove some space within the node
            Position count = stop_idx - start_idx;
            removed_key_count += removeRoom(start, level, start_idx, count, accum);

            if (level > 0)
            {
            	stop.moveLeft(level - 1, 0, count.get());
            }

            if (!start[level]->is_root())
            {
                removeRedundantRoot(start, stop, level);
            }

            stop_idx = start_idx;
        }
    }
    else
    {
        // The region to remove crosses node boundaries.
        // We need to up the tree until we found the node
        // enclosing the region. See the code branch above.

        removed_key_count       += removeRoom(start, level, start_idx, self.getNodeSizes(start[level].node()) - start_idx, accum);

        removed_key_count       += removeRoom(stop,  level, Position(0), stop_idx, accum);

        stop_idx.setAll(0);

        Position start_parent_idx = Position(start[level].parent_idx() + 1);

        // FIXME: stop[level].parent_idx() - can be updated elsewhere in makeRoom() - check it

        Position stop_parent_idx = Position(stop[level].parent_idx());

        removePages(start, start_parent_idx, stop, stop_parent_idx, level + 1, accum, removed_key_count);

        stop[level].parent_idx() = stop_parent_idx.get();


        if (isTheSameParent(start, stop, level))
        {
            if (canMerge(start, stop, level))
            {
                mergeNodes(start, stop, level);

                stop_idx = start_idx;

                removeRedundantRoot(start, stop, level);
            }
        }
    }
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
void M_TYPE::removePage(TreePath& path, Int& key_idx)
{
	auto& self = this->self();

	for (Int c = 1; c < path.getSize(); c++)
    {
        if (path[c]->children_count() > 1)
        {
            Accumulator accum;

            Int idx = path[c - 1].parent_idx();
            self.removeRoom(path, c, Position(idx), Position(1), accum);

            if (idx == path[c]->children_count())
            {
                if (self.getNextNode(path, c, true))
                {
                    key_idx = 0;
                }
                else
                {
                    for (Int d = c - 1; d >= 0; d--)
                    {
                        path[d].node()          = self.getLastChild(path[d + 1].node(), Allocator::READ);
                        path[d].parent_idx()    = path[d + 1]->children_count() - 1;
                    }

                    key_idx = path.leaf()->children_count();

                    self.finishPathStep(path, key_idx);
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

                self.finishPathStep(path, 0);
                key_idx = 0;
            }

            return;
        }
        else if (path[c]->is_root())
        {
        	self.removeNode(path[c].node());

            NodeBaseG node = self.createRootNode(0, true, me()->getRootMetadata());

            self.set_root(node->id());
            path.clear();
            path.append(TreePathItem(node, 0));

            key_idx = 0;

            return;
        }
    }

	self.removeNode(path.leaf().node());

    NodeBaseG node = self.createRootNode(0, true, self.getRootMetadata());

    self.set_root(node->id());
    path.clear();
    path.append(TreePathItem(node, 0));

    key_idx = 0;
}

/**
 * \brief Remove node with all its children from the tree.
 *
 * \return number of children have been removed.
 *
 * \see removePage
 */

M_PARAMS
BigInt M_TYPE::removeNode(NodeBaseG node)
{
    auto& self = this->self();

	const Int children_count = node->children_count();

    BigInt count = 0;

    if (!node->is_leaf())
    {
        for (Int c = 0; c < children_count; c++)
        {
            NodeBaseG child = self.getChild(node, c, Allocator::READ);
            count += self.removeNode(child);
        }
    }
    else {
    	Int tmp = self.getNodeSizes(node).get();
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

	if (parent.isSet() && parent->is_root() && parent->children_count() == 1)
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

        if (node->children_count() == 1)
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

        if (node->children_count() == 1)
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
bool M_TYPE::mergeWithSiblings(TreePath& path, Int level, Position& key_idx)
{
    if (self().mergeWithRightSibling(path, level))
    {
        return true;
    }
    else
    {
        return self().mergeWithLeftSibling(path, level, key_idx);
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
bool M_TYPE::mergeWithLeftSibling(TreePath& path, Int level, Position& key_idx)
{
	if (!key_idx.gteAll(0)) {
		int a = 0; a++;
	}

	MEMORIA_ASSERT_TRUE(key_idx.gteAll(0));

    auto& self = this->self();

    bool merged = false;

    if (self.shouldMergeNode(path, level))
    {
        TreePath prev = path;

        if (self.getPrevNode(prev, level))
        {
            Position size = self.getNodeSizes(prev[level]);

            merged = mergeBTreeNodes(prev, path, level);

            if (merged)
            {
                // FIXME: array subscripts?
            	key_idx += size;
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
            merged = mergeBTreeNodes(path, next, level);
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

    NodeBaseG& page1 = tgt[level].node();
    NodeBaseG& page2 = src[level].node();

    page1.update();

    Int tgt_children_count = page1->children_count();

    NodeDispatcher::dispatch2(page2, page1, MergeNodesFn());

    NodeBaseG& parent   = src[level + 1].node();
    Int parent_idx      = src[level].parent_idx();

    Accumulator accum;

    removeRoom(src, level + 1, Position(parent_idx), Position(1), accum, false);

    self.updateUp(src, level + 1, parent_idx - 1, accum);

    self.allocator().removePage(page2->id());

    src[level] = tgt[level];

    if (level > 0)
    {
    	src.moveRight(level - 1, 0, tgt_children_count);
    }

    self.reindex(parent); //FIXME: does it necessary?
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
bool M_TYPE::mergeBTreeNodes(TreePath& tgt, TreePath& src, Int level)
{
    if (canMerge(tgt, src, level))
    {
        if (isTheSameParent(tgt, src, level))
        {
            mergeNodes(tgt, src, level);

            removeRedundantRoot(tgt, src, level);

            return true;
        }
        else
        {
            if (self().mergeBTreeNodes(tgt, src, level + 1))
            {
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
