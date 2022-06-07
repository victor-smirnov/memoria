
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

#include <memoria/core/container/macros.hpp>
#include <memoria/core/tools/result.hpp>

#include <memoria/profiles/common/container_operations.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::WalkName)

    using typename Base::Types;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::Profile;
    using typename Base::CtrID;
    using typename Base::BlockID;


    void ctr_walk_tree(ContainerWalker<Profile>* walker)
    {
        auto& self = this->self();

        auto root = self.ctr_get_root_node();

        walker->beginCtr(
            TypeNameFactory<typename Types::ContainerTypeName>::name().data(),
            self.name(),
            root->id()
        );

        this->ctr_traverse_tree(root, walker);

        walker->endCtr();
    }

    // FIXME: error handling
    void ctr_begin_node(const TreeNodeConstPtr& node, ContainerWalker<Profile>* walker)
    {
        if (node->is_root())
        {
            if (node->is_leaf())
            {
                walker->rootLeaf(node.block()->as_header());
            }
            else {
                walker->beginRoot(node.block()->as_header());
            }
        }
        else if (node->is_leaf())
        {
            walker->leaf(node.block()->as_header());
        }
        else {
            walker->ctr_begin_node(node.block()->as_header());
        }
    }

    // TODO: error handling
    void ctr_end_node(const TreeNodeConstPtr& node, ContainerWalker<Profile>* walker)
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

    CtrID ctr_clone(const CtrID& new_name) const
    {
        if (new_name.is_null())
        {
            new_name = IDTools<CtrID>::make_random();
        }

        auto& self = this->self();

        auto root_id = self.store().getRootID(new_name);
        if (root_id.is_null())
        {
            TreeNodeConstPtr root = self.ctr_get_root_node();

            TreeNodePtr new_root = self.ctr_clone_tree(root, root->parent_id());

            auto new_meta = self.ctr_get_ctr_root_metadata(new_root.as_immutable());

            new_meta.model_name() = new_name;

            self.ctr_set_ctr_root_metadata(new_root, new_meta);

            self.store().setRoot(new_name, new_root->id());

            return new_name;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Requested container name of {} is already in use.", new_name).do_throw();
        }
    }


private:

    TreeNodePtr ctr_clone_tree(const TreeNodeConstPtr& node, const BlockID& parent_id) const
    {
        auto& self = this->self();

        TreeNodePtr new_node = self.store().cloneBlock(node.shared());
        new_node->parent_id() = parent_id;

        if (!node->is_leaf())
        {
            self.ctr_for_all_ids(node, 0, self.ctr_get_node_size(node, 0), [&](const BlockID& id, size_t idx)
            {
                auto child = self.ctr_get_block(id);
                TreeNodePtr new_child = self.ctr_clone_tree(child, new_node->id());
                self.ctr_set_child_id(new_node, idx, new_child->id());
            });
        }

        return new_node;
    }

    void ctr_traverse_tree(const TreeNodeConstPtr& node, ContainerWalker<Profile>* walker)
    {
        auto& self = this->self();

        self.ctr_begin_node(node, walker);

        if (!node->is_leaf())
        {
            auto node_size = self.ctr_get_node_size(node, 0);

            self.ctr_for_all_ids(node, 0, node_size, [&self, walker](const BlockID& id) {
                auto child = self.ctr_get_block(id);
                return self.ctr_traverse_tree(child, walker);
            });
        }

        self.ctr_end_node(node, walker);
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::WalkName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
