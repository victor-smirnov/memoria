
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

#include <iostream>

#include <memoria/v1/core/container/logs.hpp>
#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/tools/result.hpp>


namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::WalkName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    using typename Base::BlockID;
    using typename Base::CtrID;

    using ProfileT = typename Types::Profile;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    using BranchNodeEntry = typename Types::BranchNodeEntry;


    VoidResult ctr_walk_tree(ContainerWalker<ProfileT>* walker) noexcept
    {
        auto& self = this->self();

        Result<NodeBaseG> root = self.ctr_get_root_node();
        MEMORIA_RETURN_IF_ERROR(root);

        walker->beginCtr(
            TypeNameFactory<typename Types::ContainerTypeName>::name().data(),
            self.name(),
            root.get()->id()
        );

        MEMORIA_RETURN_IF_ERROR_FN(this->ctr_traverse_tree(root.get(), walker));

        walker->endCtr();
        return VoidResult::of();
    }

    // TODO: error handling
    void ctr_begin_node(const NodeBaseG& node, ContainerWalker<ProfileT>* walker) noexcept
    {
        if (node->is_root())
        {
            if (node->is_leaf())
            {
                walker->rootLeaf(node->parent_idx(), node.block()->as_header());
            }
            else {
                walker->beginRoot(node->parent_idx(), node.block()->as_header());
            }
        }
        else if (node->is_leaf())
        {
            walker->leaf(node->parent_idx(), node.block()->as_header());
        }
        else {
            walker->ctr_begin_node(node->parent_idx(), node.block()->as_header());
        }
    }

    // TODO: error handling
    void ctr_end_node(const NodeBaseG& node, ContainerWalker<ProfileT>* walker) noexcept
    {
        if (node->is_root())
        {
            if (!node->is_leaf())
            {
                walker->endRoot();
            }
        }
        else if (!node->is_leaf())
        {
            walker->ctr_end_node();
        }
    }

    Result<CtrID> ctr_clone(CtrID new_name) const noexcept
    {
        if (new_name.is_null())
        {
            new_name = IDTools<CtrID>::make_random();
        }

        auto& self = this->self();

        Result<BlockID> root_id = self.store().getRootID(new_name);
        MEMORIA_RETURN_IF_ERROR(root_id);

        if (root_id.get().is_null())
        {
            NodeBaseG root = self.ctr_get_root_node();

            NodeBaseG new_root = self.ctr_clone_tree(root, root->parent_id());

            auto new_meta = self.ctr_get_ctr_root_metadata(new_root);

            new_meta.model_name() = new_name;

            self.ctr_set_ctr_root_metadata(new_root, new_meta);

            self.store().setRoot(new_name, new_root->id());

            return new_name;
        }
        else {
            return Result<CtrID>::make_error("Requested container name of {} is already in use.", new_name);
        }
    }


private:

    Result<NodeBaseG> ctr_clone_tree(const NodeBaseG& node, const BlockID& parent_id) const noexcept
    {
        auto& self = this->self();

        NodeBaseG new_node = self.store().cloneBlock(node.shared(), BlockID{});
        new_node->parent_id() = parent_id;

        if (!node->is_leaf())
        {
            self.ctr_for_all_ids(node, 0, self.ctr_get_node_size(node, 0), [&](const BlockID& id, int32_t idx)
            {
                NodeBaseG child = self.store().getBlock(id).get_or_terminate();
                NodeBaseG new_child = self.ctr_clone_tree(child, new_node->id());
                self.ctr_set_child_id(new_node, idx, new_child->id());
            });
        }

        return new_node;
    }

    VoidResult ctr_traverse_tree(const NodeBaseG& node, ContainerWalker<ProfileT>* walker) noexcept
    {
        auto& self = this->self();

        self.ctr_begin_node(node, walker);

        if (!node->is_leaf())
        {
            auto res1 = self.ctr_for_all_ids(node, 0, self.ctr_get_node_size(node, 0), [&self, walker](const BlockID& id, int32_t idx) noexcept
            {
                NodeBaseG child = self.store().getBlock(id).get_or_terminate();
                return self.ctr_traverse_tree(child, walker);
            });

            MEMORIA_RETURN_IF_ERROR(res1);
        }

        self.ctr_end_node(node, walker);

        return VoidResult::of();
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::WalkName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}}
