
// Copyright 2013 Victor Smirnov
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

using namespace v1::bt;
using namespace v1::core;

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bt::BranchVariableName)

public:
    using Types = typename Base::Types;

protected:
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

    static const int32_t Streams = Types::Streams;

public:
    void update_path(const NodeBaseG& node);

public:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(InsertFn, insert, OpStatus);
    OpStatus insertToBranchNodeP(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys, const ID& id);

    NodeBaseG splitPathP(NodeBaseG& node, int32_t split_at);
    NodeBaseG splitP(NodeBaseG& node, SplitFn split_fn);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(UpdateNodeFn, updateUp, OpStatus);


    MMA1_NODISCARD bool updateBranchNode(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry);

    void updateBranchNodes(NodeBaseG& node, int32_t& idx, const BranchNodeEntry& entry);

    void updateBranchNodesNoBackup(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(TryMergeNodesFn, mergeWith, OpStatus);
    bool tryMergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeCurrentBranchNodes(NodeBaseG& tgt, NodeBaseG& src);


MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bt::BranchVariableName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
OpStatus M_TYPE::insertToBranchNodeP(
        NodeBaseG& node,
        int32_t idx,
        const BranchNodeEntry& sums,
        const ID& id
)
{
    auto& self = this->self();

    self.updatePageG(node);

    if(isFail(BranchDispatcher::dispatch(node, InsertFn(), idx, sums, id))) {
        return OpStatus::FAIL;
    }

    self.updateChildren(node, idx);

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);
        int32_t parent_idx = node->parent_idx();

        auto max = self.max(node);
        self.updateBranchNodes(parent, parent_idx, max);
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
    else {
        self.updatePageG(left_node);
    }

    NodeBaseG left_parent = self.getNodeParentForUpdate(left_node);

    NodeBaseG right_node = self.createNode(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

    split_fn(left_node, right_node);

    auto left_max  = self.max(left_node);
    auto right_max = self.max(right_node);

    int32_t parent_idx = left_node->parent_idx();

    self.updateBranchNodes(left_parent, parent_idx, left_max);

    PageUpdateMgr mgr(self);
    mgr.add(left_parent);

    if(isFail(self.insertToBranchNodeP(left_parent, parent_idx + 1, right_max, right_node->id())))
    {
        mgr.rollback();

        NodeBaseG right_parent = splitPathP(left_parent, parent_idx + 1);

        mgr.add(right_parent);

        if(isFail(self.insertToBranchNodeP(right_parent, 0, right_max, right_node->id())))
        {
            mgr.rollback();

            int32_t right_parent_size = self.getNodeSize(right_parent, 0);

            splitPathP(right_parent, right_parent_size / 2);

            OOM_THROW_IF_FAILED(self.insertToBranchNodeP(right_parent, 0, right_max, right_node->id()), MMA1_SRC);
        }
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
bool M_TYPE::updateBranchNode(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);
    mgr.add(node);

    if (isFail(BranchDispatcher::dispatch(node, UpdateNodeFn(), idx, entry))) {
        mgr.rollback();
        return false;
    }

    return true;
}





M_PARAMS
void M_TYPE::updateBranchNodes(NodeBaseG& node, int32_t& idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    self.updatePageG(node);

    if (!self.updateBranchNode(node, idx, entry))
    {
        int32_t size        = self.getNodeSize(node, 0);
        int32_t split_idx   = size / 2;

        NodeBaseG right = self.splitPathP(node, split_idx);

        if (idx >= split_idx)
        {
            idx -= split_idx;
            node = right;
        }

        bool result = self.updateBranchNode(node, idx, entry);
        MEMORIA_V1_ASSERT_TRUE(result);
    }

    if(!node->is_root())
    {
        int32_t parent_idx = node->parent_idx();
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        auto max = self.max(node);
        self.updateBranchNodes(parent, parent_idx, max);
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



M_PARAMS
void M_TYPE::updateBranchNodesNoBackup(NodeBaseG& node, int32_t idx, const BranchNodeEntry& entry)
{
    auto& self = this->self();

    self.updateBranchNode(node, idx, entry);

    if(!node->is_root())
    {
        int32_t parent_idx = node->parent_idx();
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        self.updateBranchNodesNoBackup(parent, parent_idx, entry);
    }
}


M_PARAMS
bool M_TYPE::tryMergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);

    self.updatePageG(src);
    self.updatePageG(tgt);

    mgr.add(src);
    mgr.add(tgt);


    int32_t tgt_size            = self.getNodeSize(tgt, 0);
    NodeBaseG src_parent    = self.getNodeParent(src);
    int32_t parent_idx          = src->parent_idx();

    MEMORIA_V1_ASSERT(parent_idx, >, 0);

    if (isFail(BranchDispatcher::dispatch(src, tgt, TryMergeNodesFn())))
    {
        mgr.rollback();
        return false;
    }

    self.updateChildren(tgt, tgt_size);

    auto max = self.max(tgt);

    if (isFail(self.removeNonLeafNodeEntry(src_parent, parent_idx))) {
        mgr.rollback();
        return false;
    }

    int32_t idx = parent_idx - 1;

    self.updateBranchNodes(src_parent, idx, max);

    self.allocator().removePage(src->id(), self.master_name());

    return true;
}


M_PARAMS
bool M_TYPE::mergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    if (self.isTheSameParent(tgt, src))
    {
        return self.mergeCurrentBranchNodes(tgt, src);
    }
    else
    {
        NodeBaseG tgt_parent = self.getNodeParent(tgt);
        NodeBaseG src_parent = self.getNodeParent(src);

        if (mergeBranchNodes(tgt_parent, src_parent))
        {
            return self.mergeCurrentBranchNodes(tgt, src);
        }
        else
        {
            return false;
        }
    }
}




M_PARAMS
bool M_TYPE::mergeCurrentBranchNodes(NodeBaseG& tgt, NodeBaseG& src)
{
    auto& self = this->self();

    if (self.tryMergeBranchNodes(tgt, src))
    {
        self.removeRedundantRootP(tgt);
        return true;
    }
    else {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS

}}
