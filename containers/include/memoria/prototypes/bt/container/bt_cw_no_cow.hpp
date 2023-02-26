
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

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::NoCoWOpsWName)

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::TreePathT;
    using typename Base::BlockID;
    using typename Base::Profile;
    using typename Base::ApiProfileT;

    void ctr_cow_clone_path(TreePathT& path, size_t level) const {
    }

    void ctr_ref_block(const TreeNodeConstPtr& block_id) {
    }

    void ctr_ref_block(const TreeNodePtr& block_id) {
    }


    void ctr_unref_block(const BlockID& block_id)
    {
        auto& self = this->self();
        auto block = self.ctr_get_block(block_id);
        return self.ctr_remove_node_recursively(block);
    }

    void ctr_resize_block(TreePathT& path, size_t level, int32_t new_size) {
        return path[level].resize(new_size);
    }

    void ctr_update_block_guard(const TreeNodePtr& node)
    {
        return node.update();
    }

    void ctr_update_block_guard(const TreeNodeConstPtr& node)
    {
        return node.update();
    }

    bool is_cascade_tree_removal() const {
        return false;
    }


    void ctr_remove_all_nodes(TreePathT& start_path, TreePathT& stop_path)
    {
        auto& self = this->self();

        TreeNodeConstPtr root = start_path.root();
        start_path.clear();
        stop_path.clear();

        auto new_root = self.ctr_create_root_node(0, true, -1);

        self.ctr_unref_block(root->id());

        self.set_root(new_root->id());

        start_path.add_root(new_root.as_immutable());

        if (stop_path.size() == 0)
        {
            stop_path.add_root(new_root.as_immutable());
        }
    }


    void ctr_remove_node_recursively(const TreeNodeConstPtr& node)
    {
        auto& self = this->self();

        if (!node->is_leaf())
        {
            self.ctr_for_all_ids(node, [&](const BlockID& id)
            {
                auto child = self.ctr_get_block(id);
                return self.ctr_remove_node_recursively(child);
            });
        }

        return self.store().removeBlock(node->id());
    }

    void drop()
    {
        auto& self = this->self();

        if (self.store().isActive())
        {
            self.for_each_ctr_reference([&](auto prop_name, auto ctr_id){
                self.store().drop_ctr(ctr_id);
            });

            auto root = self.ctr_get_root_node();
            self.ctr_remove_node_recursively(root);

            self.set_root(BlockID{});

            self.store().on_ctr_drop(self.name());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot must be in active state to drop containers").do_throw();
        }
    }

    void cleanup()
    {
        auto& self = this->self();
        auto metadata = self.ctr_get_root_metadata();

        TreeNodePtr new_root = self.ctr_create_node(0, true, true, metadata.memory_block_size());

        // FIXME: This method must preserve root metadata and
        // delegate metadata cleanup to actual containers.

        // FIXME: in case of container refcounting this trick
        // can drop container entirely.
        self.drop();
        return self.set_root(new_root->id());
    }



    void ctr_remove_redundant_root(TreePathT& path, size_t level)
    {
        auto& self = this->self();

        if (level + 1 < path.size())
        {
            auto parent = self.ctr_get_node_parent(path, level);

            if (!parent->is_root())
            {
                ctr_remove_redundant_root(path, level + 1);
            }

            if (parent->is_root())
            {
                auto size = self.ctr_get_node_size(parent, 0);
                if (size == 1)
                {
                    TreeNodeConstPtr node = path[level];

                    // FIXME redesign it to use tryConvertToRoot(node) instead
                    if (self.ctr_can_convert_to_root(node, parent->root_metadata_size()))
                    {
                        self.ctr_update_block_guard(node);

                        self.ctr_node_to_root(node);
                        self.store().removeBlock(parent->id());
                        self.set_root(node->id());

                        path.remove_root();
                    }
                }
            }
        }
    }

    void ctr_cow_ref_children_after_merge(TreeNodePtr block)
    {
    }

//    virtual void traverse_ctr(void* node_handler) const {
//        make_generic_error("Method is not implemented for this profile").do_throw();
//    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::NoCoWOpsWName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
