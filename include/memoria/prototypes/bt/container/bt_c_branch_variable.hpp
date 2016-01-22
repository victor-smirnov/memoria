
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_BRANCH_VARIABLE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_BRANCH_VARIABLE_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::BranchVariableName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef std::function<BranchNodeEntry (NodeBaseG&, NodeBaseG&)>                 SplitFn;


    static const Int Streams                                                    = Types::Streams;


    MEMORIA_DECLARE_NODE_FN(InsertFn, insert);
    void insertToBranchNodeP(NodeBaseG& node, Int idx, const BranchNodeEntry& keys, const ID& id);

    NodeBaseG splitPathP(NodeBaseG& node, Int split_at);
    NodeBaseG splitP(NodeBaseG& node, SplitFn split_fn);

    MEMORIA_DECLARE_NODE_FN(UpdateNodeFn, updateUp);

    template <typename UpdateData>
    bool updateNode(NodeBaseG& node, Int idx, const UpdateData& sums);

    template <typename UpdateData>
    void updatePath(NodeBaseG& node, Int& idx, const UpdateData& sums);

    template <typename UpdateData>
    void update_parent(NodeBaseG& node, const UpdateData& sums);

    template <typename UpdateData>
    void updatePathNoBackup(NodeBaseG& node, Int idx, const UpdateData& sums);


    MEMORIA_DECLARE_NODE_FN(TryMergeNodesFn, mergeWith);
    bool tryMergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeBranchNodes(NodeBaseG& tgt, NodeBaseG& src);
    bool mergeCurrentBranchNodes(NodeBaseG& tgt, NodeBaseG& src);


MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::BranchVariableName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::insertToBranchNodeP(
        NodeBaseG& node,
        Int idx,
        const BranchNodeEntry& sums,
        const ID& id
)
{
    auto& self = this->self();

    self.updatePageG(node);
    BranchDispatcher::dispatch(node, InsertFn(), idx, sums, id);
    self.updateChildren(node, idx);

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);
        Int parent_idx = node->parent_idx();
        self.updatePath(parent, parent_idx, sums);
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
    else {
        self.updatePageG(left_node);
    }

    NodeBaseG left_parent  = self.getNodeParentForUpdate(left_node);

    NodeBaseG other  = self.createNode1(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

    BranchNodeEntry sums = split_fn(left_node, other);

    Int parent_idx   = left_node->parent_idx();

    self.updatePathNoBackup(left_parent, parent_idx, -sums);

    PageUpdateMgr mgr(self);
    mgr.add(left_parent);

    try {
        self.insertToBranchNodeP(left_parent, parent_idx + 1, sums, other->id());
    }
    catch (PackedOOMException ex)
    {
        mgr.rollback();

        NodeBaseG right_parent = splitPathP(left_parent, parent_idx + 1);

        mgr.add(right_parent);

        try {
            self.insertToBranchNodeP(right_parent, 0, sums, other->id());
        }
        catch (PackedOOMException ex2)
        {
            mgr.rollback();

            Int right_parent_size = self.template getNodeSize(right_parent, 0);

            splitPathP(right_parent, right_parent_size / 2);

            self.insertToBranchNodeP(right_parent, 0, sums, other->id());
        }
    }

    return other;
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
template <typename UpdateData>
bool M_TYPE::updateNode(NodeBaseG& node, Int idx, const UpdateData& sums)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);

    mgr.add(node);

    try {
        BranchDispatcher::dispatch(node, UpdateNodeFn(), idx, sums);
        return true;
    }
    catch (PackedOOMException ex)
    {
        mgr.rollback();
        return false;
    }
}





M_PARAMS
template <typename UpdateData>
void M_TYPE::updatePath(NodeBaseG& node, Int& idx, const UpdateData& sums)
{
    auto& self = this->self();

    self.updatePageG(node);

    if (!self.updateNode(node, idx, sums))
    {
        Int size        = self.getNodeSize(node, 0);
        Int split_idx   = size / 2;

        NodeBaseG right = self.splitPathP(node, split_idx);

        if (idx >= split_idx)
        {
            idx -= split_idx;
            node = right;
        }

        bool result = self.updateNode(node, idx, sums);
        MEMORIA_ASSERT_TRUE(result);
    }

    if(!node->is_root())
    {
        Int parent_idx = node->parent_idx();
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        self.updatePath(parent, parent_idx, sums);
    }
}




M_PARAMS
template <typename UpdateData>
void M_TYPE::update_parent(NodeBaseG& node, const UpdateData& sums)
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);
        Int parent_idx = node->parent_idx();
        self.updatePath(parent, parent_idx, sums);
    }
}




M_PARAMS
template <typename UpdateData>
void M_TYPE::updatePathNoBackup(NodeBaseG& node, Int idx, const UpdateData& sums)
{
    auto& self = this->self();

    self.updateNode(node, idx, sums);

    if(!node->is_root())
    {
        Int parent_idx = node->parent_idx();
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        self.updatePathNoBackup(parent, parent_idx, sums);
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

    try {
        Int tgt_size            = self.getNodeSize(tgt, 0);
        NodeBaseG src_parent    = self.getNodeParent(src);
        Int parent_idx          = src->parent_idx();

        MEMORIA_ASSERT(parent_idx, >, 0);

        BranchDispatcher::dispatch(src, tgt, TryMergeNodesFn());

        self.updateChildren(tgt, tgt_size);

        BranchNodeEntry sums        = self.sums(src_parent, parent_idx, parent_idx + 1);

        self.removeNonLeafNodeEntry(src_parent, parent_idx);

        Int idx = parent_idx - 1;

        self.updatePath(src_parent, idx, sums);

        self.allocator().removePage(src->id(), self.master_name());

        return true;
    }
    catch (PackedOOMException ex)
    {
        mgr.rollback();
    }

    return false;
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

}



#endif
