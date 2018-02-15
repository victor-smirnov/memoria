
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

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bt::RemoveToolsName)

    typedef TypesType                                                           Types;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::ID                                                   ID;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Base::Metadata                                             Metadata;

    typedef std::function<void (const Position&)>                               MergeFn;

    using Base::CONTAINER_HASH;


public:
    void drop()
    {
        auto& self = this->self();

        if (self.allocator().isActive())
        {
            auto meta = self.getRootMetadata();

            for (int32_t c = 0; c < meta.ROOTS; c++)
            {
                const auto& root = meta.roots(UUID(0, c));
                if (!root.is_null())
                {
                    auto root_page      = self.allocator().getPage(root, UUID());
                    auto ctr_meta_rep   = MetadataRepository<typename Types::Profile>::getMetadata();

                    int32_t ctr_hash        = root_page->ctr_type_hash();

                    auto ctr_meta       = ctr_meta_rep->getContainerMetadata(ctr_hash);

                    auto ctr_interface  = ctr_meta->getCtrInterface();

                    ctr_interface->drop(root, UUID(), self.allocator().self_ptr());
                }
            }

            NodeBaseG root = self.getRoot();
            self.removeRootNode(root);
            self.set_root(ID());
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Transaction must be in active state to drop containers");
        }
    }


protected:
    MEMORIA_V1_DECLARE_NODE_FN(RemoveSpaceFn, removeSpace);

    void removeNodeRecursively(NodeBaseG& node, Position& accum);
    void removeNode(NodeBaseG& node);
    void removeRootNode(NodeBaseG& node);

    void removeNodeContent(NodeBaseG& node, int32_t start, int32_t end, Position& sums);
    Position removeLeafContent(NodeBaseG& node, const Position& start, const Position& end);
    Position removeLeafContent(NodeBaseG& node, int32_t stream, int32_t start, int32_t end);

    MEMORIA_V1_DECLARE_NODE_FN(RemoveNonLeafNodeEntryFn, removeSpaceAcc);
    void removeNonLeafNodeEntry(NodeBaseG& node, int32_t idx);

    bool mergeLeafWithLeftSibling(NodeBaseG& node, MergeFn fn = [](const Position&, int32_t){});
    bool mergeLeafWithRightSibling(NodeBaseG& node);
    MergeType mergeLeafWithSiblings(NodeBaseG& node, MergeFn fn = [](const Position&, int32_t){});


    MEMORIA_V1_DECLARE_NODE_FN_RTN(ShouldBeMergedNodeFn, shouldBeMergedWithSiblings, bool);
    bool shouldMergeNode(const NodeBaseG& node) const
    {
        return NodeDispatcher::dispatch(node, ShouldBeMergedNodeFn());
    }




    ////  ------------------------ CONTAINER PART PRIVATE API ------------------------



    void removeRedundantRootP(NodeBaseG& node);


    bool isTheSameParent(const NodeBaseG& left, const NodeBaseG& right)
    {
        return left->parent_id() == right->parent_id();
    }




MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bt::RemoveToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::removeNodeRecursively(NodeBaseG& node, Position& sizes)
{
    auto& self = this->self();

    if (!node->is_leaf())
    {
        int32_t size = self.getNodeSize(node, 0);
        self.forAllIDs(node, 0, size, [&, this](const ID& id, int32_t idx)
        {
            auto& self = this->self();
            NodeBaseG child = self.allocator().getPage(id, self.master_name());
            this->removeNodeRecursively(child, sizes);
        });
    }
    else {
        sizes += self.leaf_sizes(node);
    }

    self.allocator().removePage(node->id(), self.master_name());
}

M_PARAMS
void M_TYPE::removeNode(NodeBaseG& node)
{
    auto& self = this->self();

    BranchNodeEntry sums;
    Position sizes;

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParentForUpdate(node);

        self.removeNonLeafNodeEntry(parent, node->parent_idx());

        self.removeNode(node, sums, sizes);
    }
    else {
        MMA1_THROW(Exception()) << WhatCInfo("Empty root node should not be deleted with this method.");
    }
}

M_PARAMS
void M_TYPE::removeRootNode(NodeBaseG& node)
{
    auto& self = this->self();

    MEMORIA_V1_ASSERT_TRUE(node->is_root());

    Position sizes;

    self.removeNodeRecursively(node, sizes);
}



M_PARAMS
void M_TYPE::removeNodeContent(NodeBaseG& node, int32_t start, int32_t end, Position& sizes)
{
    auto& self = this->self();

    MEMORIA_V1_ASSERT_TRUE(!node->is_leaf());



    self.forAllIDs(node, start, end, [&, this](const ID& id, int32_t idx){
        auto& self = this->self();
        NodeBaseG child = self.allocator().getPage(id, self.master_name());
        self.removeNodeRecursively(child, sizes);
    });

    BranchDispatcher::dispatch(node, RemoveSpaceFn(), start, end);

    self.update_path(node);

    self.updateChildren(node, start);
}


M_PARAMS
void M_TYPE::removeNonLeafNodeEntry(NodeBaseG& node, int32_t start)
{
    auto& self = this->self();

    MEMORIA_V1_ASSERT_TRUE(!node->is_leaf());

    self.updatePageG(node);
    BranchDispatcher::dispatch(node, RemoveNonLeafNodeEntryFn(), start, start + 1);

    self.updateChildren(node, start);

    self.update_path(node);
}



M_PARAMS
typename M_TYPE::Position M_TYPE::removeLeafContent(NodeBaseG& node, const Position& start, const Position& end)
{
    auto& self = this->self();

    self.updatePageG(node);

    LeafDispatcher::dispatch(node, RemoveSpaceFn(), start, end);

    self.update_path(node);

    return end - start;
}

M_PARAMS
typename M_TYPE::Position M_TYPE::removeLeafContent(NodeBaseG& node, int32_t stream, int32_t start, int32_t end)
{
    auto& self = this->self();

    self.updatePageG(node);

    LeafDispatcher::dispatch(node, RemoveSpaceFn(), stream, start, end);

    self.update_path(node);

    return Position(end - start);
}



M_PARAMS
void M_TYPE::removeRedundantRootP(NodeBaseG& node)
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParent(node);
        if (!parent->is_root())
        {
            removeRedundantRootP(parent);
        }

        if (parent->is_root())
        {
            int32_t size = self.getNodeSize(parent, 0);
            if (size == 1)
            {
                Metadata root_metadata = self.getRootMetadata();

                // FIXME redesigne it to use tryConvertToRoot(node) instead
                if (self.canConvertToRoot(node))
                {
                    self.node2Root(node, root_metadata);

                    self.allocator().removePage(parent->id(), self.master_name());

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
MergeType M_TYPE::mergeLeafWithSiblings(NodeBaseG& node, MergeFn fn)
{
    auto& self = this->self();

    if (self.mergeLeafWithRightSibling(node))
    {
        return MergeType::RIGHT;
    }
    else if (self.mergeLeafWithLeftSibling(node, fn))
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
bool M_TYPE::mergeLeafWithLeftSibling(NodeBaseG& node, MergeFn fn)
{
    auto& self = this->self();

    bool merged = false;

    if (self.shouldMergeNode(node))
    {
        auto prev = self.getPrevNodeP(node);

        if (prev)
        {
            merged = self.mergeLeafNodes(prev, node, fn);

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
bool M_TYPE::mergeLeafWithRightSibling(NodeBaseG& node)
{
    bool merged = false;

    auto& self = this->self();

    if (self.shouldMergeNode(node))
    {
        auto next = self.getNextNodeP(node);

        if (next)
        {
            merged = self.mergeLeafNodes(node, next);
        }
    }

    return merged;
}







#undef M_TYPE
#undef M_PARAMS


}}
