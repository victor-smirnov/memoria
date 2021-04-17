
// Copyright 2020 Victor Smirnov
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
#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/container/macros.hpp>


#include <vector>
#include <utility>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::NoCoWOpsName)

    using typename Base::NodeBasePtr;
    using typename Base::TreePathT;
    using typename Base::BlockID;
    using typename Base::Profile;
    using typename Base::ApiProfileT;

    VoidResult ctr_cow_clone_path(TreePathT& path, size_t level) const noexcept {
        return VoidResult::of();
    }

    VoidResult ctr_ref_block(const BlockID& block_id) noexcept
    {
        return VoidResult::of();
    }


    VoidResult ctr_unref_block(const BlockID& block_id) noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(block, self.ctr_get_block(block_id));
        return self.ctr_remove_node_recursively(block);
    }

    VoidResult ctr_resize_block(TreePathT& path, size_t level, int32_t new_size) noexcept {
        return path[level].resize(new_size);
    }

    VoidResult ctr_update_block_guard(NodeBasePtr& node) noexcept
    {
        return node.update();
    }

    bool is_cascade_tree_removal() const noexcept {
        return false;
    }


    VoidResult ctr_remove_all_nodes(TreePathT& start_path, TreePathT& stop_path) noexcept
    {
        auto& self = this->self();

        NodeBasePtr root = start_path.root();
        start_path.clear();
        stop_path.clear();

        MEMORIA_TRY(new_root, self.ctr_create_root_node(0, true, -1));

        MEMORIA_TRY_VOID(self.ctr_unref_block(root->id()));

        MEMORIA_TRY_VOID(self.set_root(new_root->id()));

        start_path.add_root(new_root);

        if (stop_path.size() == 0)
        {
            stop_path.add_root(new_root);
        }

        return VoidResult::of();
    }


    VoidResult ctr_remove_node_recursively(NodeBasePtr& node) noexcept
    {
        auto& self = this->self();

        if (!node->is_leaf())
        {
            auto res = self.ctr_for_all_ids(node, [&](const BlockID& id) noexcept -> VoidResult
            {
                MEMORIA_TRY(child, self.ctr_get_block(id));
                return self.ctr_remove_node_recursively(child);
            });

            MEMORIA_RETURN_IF_ERROR(res);
        }

        return self.store().removeBlock(node->id());
    }

    virtual VoidResult internal_unref_cascade(const ApiProfileBlockID<ApiProfileT>& root) noexcept {
        return MEMORIA_MAKE_GENERIC_ERROR("unref_cascade(BlockID) should not be called fo this profile");
    }


    VoidResult drop() noexcept
    {
        auto& self = this->self();

        if (self.store().isActive())
        {

            auto res0 = self.for_each_ctr_reference([&](auto prop_name, auto ctr_id) noexcept -> VoidResult {
                    MEMORIA_TRY_VOID(self.store().drop_ctr(ctr_id));
                    return VoidResult::of();
            });
            MEMORIA_RETURN_IF_ERROR(res0);


            MEMORIA_TRY(root, self.ctr_get_root_node());
            MEMORIA_TRY_VOID(self.ctr_remove_node_recursively(root));

            MEMORIA_TRY_VOID(self.set_root(BlockID{}));


            if (this->do_unregister_)
            {
                this->do_unregister_on_dtr_ = false;
                return self.store().unregisterCtr(self.name(), this);
            }
            else {
                return VoidResult::of();
            }
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot must be in active state to drop containers");
        }
    }

    VoidResult cleanup() noexcept
    {
        auto& self = this->self();
        auto metadata = self.ctr_get_root_metadata();

        NodeBasePtr new_root = self.ctr_create_node(0, true, true, metadata.memory_block_size());

        // FIXME: This method must preserve root metadata and
        // deligate metadata cleanup to actual containers.

        // FIXME: in case of container refcounting this trick
        // can drop counter entirely.
        self.drop();
        return self.set_root(new_root->id());
    }



    VoidResult ctr_remove_redundant_root(TreePathT& path, size_t level) noexcept
    {
        auto& self = this->self();

        if (level + 1 < path.size())
        {
            MEMORIA_TRY(parent, self.ctr_get_node_parent(path, level));

            if (!parent->is_root())
            {
                MEMORIA_TRY_VOID(ctr_remove_redundant_root(path, level + 1));
            }

            if (parent->is_root())
            {
                MEMORIA_TRY(size, self.ctr_get_node_size(parent, 0));
                if (size == 1)
                {
                    NodeBasePtr node = path[level];

                    // FIXME redesign it to use tryConvertToRoot(node) instead
                    if (self.ctr_can_convert_to_root(node, parent->root_metadata_size()))
                    {
                        MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

                        MEMORIA_TRY_VOID(self.ctr_node_to_root(node));
                        MEMORIA_TRY_VOID(self.store().removeBlock(parent->id()));
                        MEMORIA_TRY_VOID(self.set_root(node->id()));

                        path.remove_root();
                    }
                }
            }
        }

        return VoidResult::of();
    }

    VoidResult ctr_cow_ref_children_after_merge(NodeBasePtr block) noexcept
    {
        return VoidResult::of();
    }

    virtual VoidResult traverse_ctr(void* node_handler) const noexcept {
        return make_generic_error("Method is not implemented for this profile");
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::NoCoWOpsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
