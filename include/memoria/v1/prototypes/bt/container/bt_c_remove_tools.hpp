
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

#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>


#include <functional>



namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::RemoveToolsName)

    typedef TypesType                                                           Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;
    using typename Base::CtrID;

    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;

    typedef std::function<void (const Position&)>                               MergeFn;

    using Base::CONTAINER_HASH;


public:
    void drop()
    {
        auto& self = this->self();

        if (self.store().isActive())
        {
            self.for_each_ctr_reference([&](auto prop_name, auto ctr_id){
                self.store().drop_ctr(ctr_id);
            });

            NodeBaseG root = self.ctr_get_root_node();
            self.ctr_remove_root_node(root);
            self.set_root(BlockID{});

            this->do_unregister_on_dtr_ = false;
            self.store().unregisterCtr(self.name(), this);
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Transaction must be in active state to drop containers");
        }
    }


    void cleanup()
    {
        auto& self = this->self();
        auto metadata = self.ctr_get_root_metadata();

        NodeBaseG new_root = self.ctr_create_node(0, true, true, metadata.memory_block_size());

        self.drop();
        self.set_root(new_root->id());
    }

protected:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(RemoveSpaceFn, removeSpace, OpStatus);

    void ctr_remove_node_recursively(NodeBaseG& node, Position& accum);
    void ctr_remove_rode(NodeBaseG& node);
    void ctr_remove_root_node(NodeBaseG& node);

    void ctr_remove_leaf_content(NodeBaseG& node, int32_t start, int32_t end, Position& sums);
    Position ctr_remove_leaf_content(NodeBaseG& node, const Position& start, const Position& end);
    Position ctr_remove_leaf_content(NodeBaseG& node, int32_t stream, int32_t start, int32_t end);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(RemoveNonLeafNodeEntryFn, removeSpaceAcc, OpStatus);
    OpStatus ctr_remove_non_leaf_node_entry(NodeBaseG& node, int32_t idx);

    bool ctr_merge_leaf_with_left_sibling(NodeBaseG& node, MergeFn fn = [](const Position&, int32_t){});
    bool ctr_merge_leaf_with_right_sibling(NodeBaseG& node);
    MergeType ctr_merge_leaf_with_siblings(NodeBaseG& node, MergeFn fn = [](const Position&, int32_t){});


    MEMORIA_V1_DECLARE_NODE_FN_RTN(ShouldBeMergedNodeFn, shouldBeMergedWithSiblings, bool);
    bool ctr_should_merge_node(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, ShouldBeMergedNodeFn());
    }




    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------



    void ctr_remove_redundant_root(NodeBaseG& node);


    bool ctr_is_the_same_parent(const NodeBaseG& left, const NodeBaseG& right)
    {
        return left->parent_id() == right->parent_id();
    }




MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::RemoveToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::ctr_remove_node_recursively(NodeBaseG& node, Position& sizes)
{
    auto& self = this->self();

    if (!node->is_leaf())
    {
        int32_t size = self.ctr_get_node_size(node, 0);
        self.ctr_for_all_ids(node, 0, size, [&, this](const BlockID& id, int32_t idx)
        {
            auto& self = this->self();
            NodeBaseG child = self.store().getBlock(id);
            this->ctr_remove_node_recursively(child, sizes);
        });
    }
    else {
        sizes += self.ctr_leaf_sizes(node);
    }

    self.store().removeBlock(node->id());
}

M_PARAMS
void M_TYPE::ctr_remove_rode(NodeBaseG& node)
{
    auto& self = this->self();

    BranchNodeEntry sums;
    Position sizes;

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent_for_update(node);

        self.ctr_remove_non_leaf_node_entry(parent, node->parent_idx());

        self.ctr_remove_rode(node, sums, sizes);
    }
    else {
        MMA1_THROW(Exception()) << WhatCInfo("Empty root node should not be deleted with this method.");
    }
}

M_PARAMS
void M_TYPE::ctr_remove_root_node(NodeBaseG& node)
{
    auto& self = this->self();

    MEMORIA_V1_ASSERT_TRUE(node->is_root());

    Position sizes;

    self.ctr_remove_node_recursively(node, sizes);
}



M_PARAMS
void M_TYPE::ctr_remove_leaf_content(NodeBaseG& node, int32_t start, int32_t end, Position& sizes)
{
    auto& self = this->self();

    MEMORIA_V1_ASSERT_TRUE(!node->is_leaf());

    self.ctr_for_all_ids(node, start, end, [&, this](const BlockID& id, int32_t idx){
        auto& self = this->self();
        NodeBaseG child = self.store().getBlock(id);
        self.ctr_remove_node_recursively(child, sizes);
    });

    OOM_THROW_IF_FAILED(self.branch_dispatcher().dispatch(node, RemoveSpaceFn(), start, end), MMA1_SRC);

    self.ctr_update_path(node);

    self.ctr_update_children(node, start);
}


M_PARAMS
OpStatus M_TYPE::ctr_remove_non_leaf_node_entry(NodeBaseG& node, int32_t start)
{
    auto& self = this->self();

    MEMORIA_V1_ASSERT_TRUE(!node->is_leaf());

    self.ctr_update_block_guard(node);
    if (isFail(self.branch_dispatcher().dispatch(node, RemoveNonLeafNodeEntryFn(), start, start + 1))) {
        return OpStatus::FAIL;
    }

    self.ctr_update_children(node, start);

    self.ctr_update_path(node);

    return OpStatus::OK;
}



M_PARAMS
typename M_TYPE::Position M_TYPE::ctr_remove_leaf_content(NodeBaseG& node, const Position& start, const Position& end)
{
    auto& self = this->self();

    self.ctr_update_block_guard(node);

    OOM_THROW_IF_FAILED(self.leaf_dispatcher().dispatch(node, RemoveSpaceFn(), start, end), MMA1_SRC);

    self.ctr_update_path(node);

    return end - start;
}

M_PARAMS
typename M_TYPE::Position M_TYPE::ctr_remove_leaf_content(NodeBaseG& node, int32_t stream, int32_t start, int32_t end)
{
    auto& self = this->self();

    self.ctr_update_block_guard(node);

    OOM_THROW_IF_FAIL(self.leaf_dispatcher().dispatch(node, RemoveSpaceFn(), stream, start, end), MMA1_SRC);

    self.ctr_update_path(node);

    return Position(end - start);
}



M_PARAMS
void M_TYPE::ctr_remove_redundant_root(NodeBaseG& node)
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.ctr_get_node_parent(node);
        if (!parent->is_root())
        {
            ctr_remove_redundant_root(parent);
        }

        if (parent->is_root())
        {
            int32_t size = self.ctr_get_node_size(parent, 0);
            if (size == 1)
            {
                Metadata root_metadata = self.ctr_get_root_metadata();

                // FIXME redesigne it to use tryConvertToRoot(node) instead
                if (self.ctr_can_convert_to_root(node, parent->root_metadata_size()))
                {
                    self.ctr_node_to_root(node, root_metadata);

                    self.store().removeBlock(parent->id());

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
MergeType M_TYPE::ctr_merge_leaf_with_siblings(NodeBaseG& node, MergeFn fn)
{
    auto& self = this->self();

    if (self.ctr_merge_leaf_with_right_sibling(node))
    {
        return MergeType::RIGHT;
    }
    else if (self.ctr_merge_leaf_with_left_sibling(node, fn))
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
 * Calls \ref ctr_should_merge_node to check if requested node should be merged with its left sibling, then merge if true.
 *
 * \param path path to the node
 * \param level level at the tree of the node
 * \param key_idx some key index in the merging node. After merge the value will be incremented with the
 * size of the merged sibling.
 *
 * \return true if node has been merged
 *
 * \see mergeWithRightSibling, ctr_should_merge_node for details
 */


M_PARAMS
bool M_TYPE::ctr_merge_leaf_with_left_sibling(NodeBaseG& node, MergeFn fn)
{
    auto& self = this->self();

    bool merged = false;

    if (self.ctr_should_merge_node(node))
    {
        auto prev = self.ctr_get_prev_node(node);

        if (prev)
        {
            merged = self.ctr_merge_leaf_nodes(prev, node, fn);

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
 * Calls \ref ctr_should_merge_node to check if requested node should be merged with its right sibling, then merge if true.
 *
 * \param path path to the node
 * \param level level of the node in the tree

 * \return true if node has been merged
 *
 * \see mergeWithLeftSibling, ctr_should_merge_node for details
 */

M_PARAMS
bool M_TYPE::ctr_merge_leaf_with_right_sibling(NodeBaseG& node)
{
    bool merged = false;

    auto& self = this->self();

    if (self.ctr_should_merge_node(node))
    {
        auto next = self.ctr_get_next_node(node);

        if (next)
        {
            merged = self.ctr_merge_leaf_nodes(node, next);
        }
    }

    return merged;
}







#undef M_TYPE
#undef M_PARAMS


}}
