
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::BranchFixedName)

public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef std::function<void (NodeBaseG&, NodeBaseG&)>                        SplitFn;

    static const Int Streams                                                    = Types::Streams;

public:
    void update_parent(NodeBaseG& node, const BranchNodeEntry& entry);

protected:
    MEMORIA_DECLARE_NODE_FN(InsertFn, insert);
    void insertToBranchNodeP(NodeBaseG& node, Int idx, const BranchNodeEntry& keys, const ID& id);

    NodeBaseG splitPathP(NodeBaseG& node, Int split_at);

    NodeBaseG splitP(NodeBaseG& node, SplitFn split_fn);

    MEMORIA_DECLARE_NODE_FN(UpdateNodeFn, updateUp);
    bool updateBranchNode(NodeBaseG& node, Int idx, const BranchNodeEntry& entry);

    void updateBranchNodes(NodeBaseG& node, Int& idx, const BranchNodeEntry& entry);





    MEMORIA_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);
    bool canMerge(const NodeBaseG& tgt, const NodeBaseG& src)
    {
        return NodeDispatcher::dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_DECLARE_NODE_FN(MergeNodesFn, mergeWith);
    void doMergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeCurrentBranchNodes(NodeBaseG& tgt, NodeBaseG& src);


MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::BranchFixedName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::insertToBranchNodeP(NodeBaseG& node, Int idx, const BranchNodeEntry& keys, const ID& id)
{
    auto& self = this->self();

    self.updatePageG(node);
    BranchDispatcher::dispatch(node, InsertFn(), idx, keys, id);
    self.updateChildren(node, idx);

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);
        auto max = self.max(node);
        self.updateBranchNodes(parent, node->parent_idx(), max);
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

    self.updatePageG(left_node);
    NodeBaseG left_parent = self.getNodeParentForUpdate(left_node);

    NodeBaseG right_node = self.createNode(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

    split_fn(left_node, right_node);

    auto left_max  = self.max(left_node);
    auto right_max = self.max(right_node);

    Int parent_idx = left_node->parent_idx();

    self.updateBranchNodes(left_parent, parent_idx, left_max);

    if (self.getBranchNodeCapacity(left_parent, -1) > 0)
    {
        self.insertToBranchNodeP(left_parent, parent_idx + 1, right_max, right_node->id());
    }
    else {
        NodeBaseG right_parent = splitPathP(left_parent, parent_idx + 1);

        self.insertToBranchNodeP(right_parent, 0, right_max, right_node->id());
    }

    return right_node;
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::splitPathP(NodeBaseG& left_node, Int split_at)
{
    auto& self = this->self();

    return splitP(left_node, [&self, split_at](NodeBaseG& left, NodeBaseG& right){
        return self.splitBranchNode(left, right, split_at);
    });
}


M_PARAMS
bool M_TYPE::updateBranchNode(NodeBaseG& node, Int idx, const BranchNodeEntry& keys)
{
    self().updatePageG(node);
    BranchDispatcher::dispatch(node, UpdateNodeFn(), idx, keys);
    return true;
}




M_PARAMS
void M_TYPE::updateBranchNodes(NodeBaseG& node, Int& idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    NodeBaseG tmp = node;

    self.updateBranchNode(tmp, idx, entry);

    while(!tmp->is_root())
    {
        auto max        = self.max(tmp);
        Int parent_idx  = tmp->parent_idx();

        tmp = self.getNodeParentForUpdate(tmp);

        self.updateBranchNode(tmp, parent_idx, max);
    }
}





M_PARAMS
void M_TYPE::update_parent(NodeBaseG& node, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        Int parent_idx = node->parent_idx();

        self.updateBranchNodes(parent, parent_idx, entry);
    }
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
void M_TYPE::doMergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    self.updatePageG(tgt);
    self.updatePageG(src);

    Int tgt_size = self.getNodeSize(tgt, 0);

    BranchDispatcher::dispatch(src, tgt, MergeNodesFn());

    self.updateChildren(tgt, tgt_size);

    NodeBaseG src_parent = self.getNodeParent(src);
    Int parent_idx       = src->parent_idx();

    MEMORIA_ASSERT(parent_idx, >, 0);

    auto max = self.max(tgt);

    self.removeNonLeafNodeEntry(src_parent, parent_idx);

    Int idx = parent_idx - 1;

    self.updateBranchNodes(src_parent, idx, max);

    self.allocator().removePage(src->id(), self.master_name());
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
bool M_TYPE::mergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    if (self.canMerge(tgt, src))
    {
        if (self.isTheSameParent(tgt, src))
        {
            self.doMergeBranchNodes(tgt, src);

            self.removeRedundantRootP(tgt);

            return true;
        }
        else
        {
            NodeBaseG tgt_parent = self.getNodeParent(tgt);
            NodeBaseG src_parent = self.getNodeParent(src);

            if (mergeBranchNodes(tgt_parent, src_parent))
            {
                self.doMergeBranchNodes(tgt, src);

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



M_PARAMS
bool M_TYPE::mergeCurrentBranchNodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    if (self.canMerge(tgt, src))
    {
        self.doMergeBranchNodes(tgt, src);

        self.removeRedundantRootP(tgt);

        return true;
    }
    else
    {
        return false;
    }
}




#undef M_TYPE
#undef M_PARAMS

} //memoria

