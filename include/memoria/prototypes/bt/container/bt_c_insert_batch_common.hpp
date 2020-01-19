
// Copyright 2015 Victor Smirnov
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>
#include <algorithm>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::InsertBatchCommonName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;
    
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                       BlockUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    class Checkpoint {
        NodeBaseG head_;
        int32_t size_;
    public:
        Checkpoint(NodeBaseG head, int32_t size): head_(head), size_(size) {}

        NodeBaseG head() const {return head_;}
        int32_t size() const {return size_;}
    };


    struct ILeafProvider {
        virtual Result<NodeBaseG> get_leaf() noexcept = 0;

        virtual Checkpoint checkpoint() = 0;

        virtual void rollback(const Checkpoint& chekpoint) = 0;

        virtual CtrSizeT size() const = 0;
    };



    VoidResult ctr_update_child_indexes(NodeBaseG& node, int32_t start) noexcept
    {
        auto& self = this->self();
        int32_t size = self.ctr_get_branch_node_size(node);

        if (start < size)
        {
            return self.ctr_for_all_ids(node, start, size, [&, this](const BlockID& id, int32_t parent_idx) noexcept -> VoidResult
            {
                auto& self = this->self();
                Result<NodeBaseG> child = static_cast_block<NodeBaseG>(self.store().getBlockForUpdate(id));
                MEMORIA_RETURN_IF_ERROR(child);

                child.get()->parent_idx() = parent_idx;
                return VoidResult::of();
            });
        }

        return VoidResult::of();
    }

    VoidResult ctr_remove_branch_nodes(const BlockID& node_id) noexcept
    {
        auto& self = this->self();

        Result<NodeBaseG> node = static_cast_block<NodeBaseG>(self.store().getBlock(node_id));
        MEMORIA_RETURN_IF_ERROR(node);

        if (node.get()->level() > 0)
        {
            auto res = self.ctr_for_all_ids(node.get(), [&, this](const BlockID& id, int32_t idx)
            {
                auto& self = this->self();
                return self.ctr_remove_branch_nodes(id);
            });
            MEMORIA_RETURN_IF_ERROR(res);

            return self.store().removeBlock(node.get()->id());
        }

        return VoidResult::of();
    }

    class InsertBatchResult {
        int32_t idx_;
        CtrSizeT subtree_size_;
    public:
        InsertBatchResult(int32_t idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

        int32_t local_pos() const {return idx_;}
        CtrSizeT subtree_size() const {return subtree_size_;}
    };


    Result<NodeBaseG> ctr_build_subtree(ILeafProvider& provider, int32_t level) noexcept
    {
        using ResultT = Result<NodeBaseG>;

        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                Result<NodeBaseG> node = self.ctr_create_node1(level, false, false);
                MEMORIA_RETURN_IF_ERROR(node);

                self.layoutNonLeafNode(node.get(), 0xFF);

                self.ctr_insert_subtree(node.get(), 0, provider, [this, level, &provider]() -> ResultT {
                    auto& self = this->self();
                    return self.ctr_build_subtree(provider, level - 1);
                }, false);

                return node;
            }
            else {
                return provider.get_leaf();
            }
        }
        else {
            return ResultT::of();
        }
    }





    class ListLeafProvider: public ILeafProvider {
        NodeBaseG   head_;
        CtrSizeT    size_ = 0;

        MyType&     ctr_;

    public:
        ListLeafProvider(MyType& ctr, NodeBaseG head, CtrSizeT size): head_(head),  size_(size), ctr_(ctr) {}

        virtual CtrSizeT size() const
        {
            return size_;
        }

        virtual Result<NodeBaseG> get_leaf() noexcept
        {
            using ResultT = Result<NodeBaseG>;

            if (head_.isSet())
            {
                auto node = head_;

                auto res = ctr_.store().getBlock(head_->next_leaf_id());
                MEMORIA_RETURN_IF_ERROR(res);

                head_ = res.get();
                size_--;
                return node;
            }
            else {
                return ResultT::make_error("Leaf List is empty");
            }
        }

        virtual Checkpoint checkpoint() {
            return Checkpoint(head_, size_);
        }


        virtual void rollback(const Checkpoint& checkpoint)
        {
            size_   = checkpoint.size();
            head_   = checkpoint.head();
        }
    };


    VoidResult ctr_update_children(const NodeBaseG& node) noexcept;
    VoidResult ctr_update_children(const NodeBaseG& node, int32_t start) noexcept;
    VoidResult ctr_update_children(const NodeBaseG& node, int32_t start, int32_t end) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(IsEmptyFn, ctr_is_empty, bool);
    bool ctr_is_empty(const NodeBaseG& node) noexcept {
        return self().node_dispatcher().dispatch(node, IsEmptyFn());
    }

private:
    VoidResult ctr_update_children_internal(const NodeBaseG& node, int32_t start, int32_t end) noexcept;
public:


    Result<NodeBaseG> createNextLeaf(NodeBaseG& left_node) noexcept;

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::InsertBatchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
Result<typename M_TYPE::NodeBaseG> M_TYPE::createNextLeaf(NodeBaseG& left_node) noexcept
{
    using ResultT = Result<NodeBaseG>;

    auto& self = this->self();

    if (left_node->is_root())
    {
        self.ctr_create_new_root_block(left_node);
    }
    else {
        self.ctr_update_block_guard(left_node);
    }

    NodeBaseG left_parent = self.ctr_get_node_parent_for_update(left_node);

    NodeBaseG other  = self.ctr_create_node1(left_node->level(), false, left_node->is_leaf(), left_node->memory_block_size());

    other->next_leaf_id().clear();

    ListLeafProvider provider(self, other, 1);

    self.ctr_insert_subtree(left_parent, left_node->parent_idx() + 1, provider);

    return ResultT::of(other);
}







M_PARAMS
VoidResult M_TYPE::ctr_update_children(const NodeBaseG& node) noexcept
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        return self.ctr_update_children_internal(node, 0, self.ctr_get_branch_node_size(node));
    }

    return VoidResult::of();
}

M_PARAMS
VoidResult M_TYPE::ctr_update_children(const NodeBaseG& node, int32_t start) noexcept
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        MEMORIA_RETURN_IF_ERROR_FN(self.ctr_update_children_internal(node, start, self.ctr_get_branch_node_size(node)));
    }

    return VoidResult::of();
}

M_PARAMS
VoidResult M_TYPE::ctr_update_children(const NodeBaseG& node, int32_t start, int32_t end) noexcept
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        self.ctr_update_children_internal(node, start, end);
    }

    return VoidResult::of();
}


M_PARAMS
VoidResult M_TYPE::ctr_update_children_internal(const NodeBaseG& node, int32_t start, int32_t end) noexcept
{
    auto& self = this->self();

    BlockID node_id = node->id();

    return self.ctr_for_all_ids(node, start, end, [&self, &node_id](const BlockID& id, int32_t idx) noexcept -> VoidResult
    {
        Result<NodeBaseG> child = static_cast_block<NodeBaseG>(self.store().getBlockForUpdate(id));
        MEMORIA_RETURN_IF_ERROR(child);


        child.get()->parent_id()  = node_id;
        child.get()->parent_idx() = idx;

        return VoidResult::of();
    });
}


#undef M_TYPE
#undef M_PARAMS

}
