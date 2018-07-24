
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BranchFixedName)

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

    static const int32_t Streams                                                    = Types::Streams;

public:

    void update_path(const NodeBaseG& node);


public:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(InsertFn, insert, OpStatus);
    OpStatus insertToBranchNodeP(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys, const ID& id);

    NodeBaseG splitPathP(NodeBaseG& node, int32_t split_at);

    NodeBaseG splitP(NodeBaseG& node, SplitFn split_fn);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(UpdateNodeFn, updateUp, OpStatus);
    bool updateBranchNode(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry);

    void updateBranchNodes(NodeBaseG& node, int32_t& idx, const BranchNodeEntry& entry);





    MEMORIA_V1_DECLARE_NODE2_FN_RTN(CanMergeFn, canBeMergedWith, bool);
    bool canMerge(const NodeBaseG& tgt, const NodeBaseG& src)
    {
        return NodeDispatcher::dispatch(src, tgt, CanMergeFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(MergeNodesFn, mergeWith, OpStatus);
    void doMergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeCurrentBranchNodes(NodeBaseG& tgt, NodeBaseG& src);


MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::BranchFixedName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
OpStatus M_TYPE::insertToBranchNodeP(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys, const ID& id)
{
    auto& self = this->self();

    self.updatePageG(node);
    if (isFail(BranchDispatcher::dispatch(node, InsertFn(), idx, keys, id))) {
        return OpStatus::FAIL;
    }

    self.updateChildren(node, idx);

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);
        auto max = self.max(node);
        self.updateBranchNodes(parent, node->parent_idx(), max);
    }

    return OpStatus::OK;
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

    int32_t parent_idx = left_node->parent_idx();

    self.updateBranchNodes(left_parent, parent_idx, left_max);

    if (self.getBranchNodeCapacity(left_parent, -1) > 0)
    {
        OOM_THROW_IF_FAILED(self.insertToBranchNodeP(left_parent, parent_idx + 1, right_max, right_node->id()), MMA1_SRC);
    }
    else {
        NodeBaseG right_parent = splitPathP(left_parent, parent_idx + 1);

        OOM_THROW_IF_FAILED(self.insertToBranchNodeP(right_parent, 0, right_max, right_node->id()), MMA1_SRC);
    }

    return right_node;
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::splitPathP(NodeBaseG& left_node, int32_t split_at)
{
    auto& self = this->self();

    return splitP(left_node, [&self, split_at](NodeBaseG& left, NodeBaseG& right){
        return self.splitBranchNode(left, right, split_at);
    });
}


M_PARAMS
bool M_TYPE::updateBranchNode(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys)
{
    self().updatePageG(node);
    OOM_THROW_IF_FAILED(BranchDispatcher::dispatch(node, UpdateNodeFn(), idx, keys), MMA1_SRC);
    return true;
}




M_PARAMS
void M_TYPE::updateBranchNodes(NodeBaseG& node, int32_t& idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    NodeBaseG tmp = node;

    self.updateBranchNode(tmp, idx, entry);

    while(!tmp->is_root())
    {
        auto max        = self.max(tmp);
        int32_t parent_idx  = tmp->parent_idx();

        tmp = self.getNodeParentForUpdate(tmp);

        self.updateBranchNode(tmp, parent_idx, max);
    }
}




M_PARAMS
void M_TYPE::update_path(const NodeBaseG& node)
{
    auto& self = this->self();

    if (!node->is_root())
    {
        auto entry = self.max(node);

        NodeBaseG parent = self.getNodeParentForUpdate(node);

        int32_t parent_idx = node->parent_idx();

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

    int32_t tgt_size = self.getNodeSize(tgt, 0);

    OOM_THROW_IF_FAILED(BranchDispatcher::dispatch(src, tgt, MergeNodesFn()), MMA1_SRC);

    self.updateChildren(tgt, tgt_size);

    NodeBaseG src_parent = self.getNodeParent(src);
    int32_t parent_idx       = src->parent_idx();

    MEMORIA_V1_ASSERT(parent_idx, >, 0);

    auto max = self.max(tgt);

    OOM_THROW_IF_FAILED(self.removeNonLeafNodeEntry(src_parent, parent_idx), MMA1_SRC);

    int32_t idx = parent_idx - 1;

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

}}
