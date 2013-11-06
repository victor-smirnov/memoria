
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMTOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMTOOLS_HPP

#include <memoria/core/container/macros.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>


#include <functional>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::RemoveToolsName)

    typedef TypesType                                                           Types;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::ID                                                   ID;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;

    typedef std::function<void (const Position&, Int)>                          MergeFn;

    void removeNode(NodeBaseG& node, Accumulator& accum, Position& sizes);
    void removeNode(NodeBaseG& node);
    void removeRootNode(NodeBaseG& node);

    MEMORIA_DECLARE_NODE_FN(RemoveNodeContentFn, removeSpace);
    void removeNodeContent(NodeBaseG& node, Int start, Int end, Accumulator& sums, Position& sizes);

    MEMORIA_DECLARE_NODE_FN_RTN(RemoveLeafContentFn, removeSpace, Accumulator);
    Accumulator removeLeafContent(NodeBaseG& node, const Position& start, const Position& end);

    Accumulator removeLeafContent(NodeBaseG& node, Int stream, Int start, Int end);


    MEMORIA_DECLARE_NODE_FN_RTN(RemoveNonLeafNodeEntryFn, removeSpaceAcc, Accumulator);
    void removeNonLeafNodeEntry(NodeBaseG& node, Int idx);





    bool mergeWithLeftSibling(NodeBaseG& node, MergeFn fn = [](const Position&, Int){});
    bool mergeWithRightSibling(NodeBaseG& node);
    MergeType mergeWithSiblings(NodeBaseG& node, MergeFn fn = [](const Position&, Int){});


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





    bool isTheSameParent(const NodeBaseG& left, const NodeBaseG& right)
    {
        return left->parent_id() == right->parent_id();
    }



    MEMORIA_DECLARE_NODE_FN_RTN(RemoveSpaceFn, removeSpace, Accumulator);


    MEMORIA_PUBLIC void drop()
    {
        NodeBaseG root = self().getRoot(Allocator::READ);
        self().removeRootNode(root);
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::RemoveToolsName)
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
        self.sums(node, sums);
        sizes += self.getNodeSizes(node);
    }

    self.allocator().removePage(node->id());
}

M_PARAMS
void M_TYPE::removeNode(NodeBaseG& node)
{
    auto& self = this->self();

    Accumulator sums;
    Position sizes;

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);

        self.removeNonLeafNodeEntry(parent, node->parent_idx());

        self.removeNode(node, sums, sizes);
    }
    else {
        throw Exception(MA_SRC, "Empty root node should not be deleted with this method.");
    }
}

M_PARAMS
void M_TYPE::removeRootNode(NodeBaseG& node)
{
    auto& self = this->self();

    MEMORIA_ASSERT_TRUE(node->is_root());

    Accumulator sums;
    Position sizes;

//    auto ctr_name   = self.name();
//    auto root_id    = node->id();

    self.removeNode(node, sums, sizes);
}



M_PARAMS
void M_TYPE::removeNodeContent(NodeBaseG& node, Int start, Int end, Accumulator& sums, Position& sizes)
{
    auto& self = this->self();

    MEMORIA_ASSERT_TRUE(!node->is_leaf());

    Accumulator deleted_sums;

    self.forAllIDs(node, start, end, [&, this](const ID& id, Int idx){
        NodeBaseG child = self.allocator().getPage(id, Allocator::READ);
        self.removeNode(child, deleted_sums, sizes);
    });

    NonLeafDispatcher::dispatch(node, RemoveNodeContentFn(), start, end);

    VectorAdd(sums, deleted_sums);

    self.updateParent(node, -deleted_sums);

    self.updateChildren(node, start);
}


M_PARAMS
void M_TYPE::removeNonLeafNodeEntry(NodeBaseG& node, Int start)
{
    auto& self = this->self();

    MEMORIA_ASSERT_TRUE(!node->is_leaf());

    node.update();
    Accumulator sums = NonLeafDispatcher::dispatchRtn(node, RemoveNonLeafNodeEntryFn(), start, start + 1);

    self.updateChildren(node, start);

    self.updateParent(node, sums);
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeLeafContent(NodeBaseG& node, const Position& start, const Position& end)
{
    auto& self = this->self();

    node.update();

    Accumulator sums = LeafDispatcher::dispatchRtn(node, RemoveLeafContentFn(), start, end);

    self.updateParent(node, -sums);

    return sums;
}

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeLeafContent(NodeBaseG& node, Int stream, Int start, Int end)
{
    auto& self = this->self();

    node.update();

    Accumulator sums = LeafDispatcher::dispatchRtn(node, RemoveLeafContentFn(), stream, start, end);

    self.updateParent(node, -sums);

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
            Int size = self.getNodeSize(parent, 0);
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
            merged = self.mergeBTreeNodes(prev, node, fn);

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
            merged = self.mergeBTreeNodes(node, next, [](const Position&, Int){});
        }
    }

    return merged;
}







#undef M_TYPE
#undef M_PARAMS


}

#endif
