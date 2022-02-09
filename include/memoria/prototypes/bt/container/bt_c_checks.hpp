
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

#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ChecksName)

    using typename Base::BlockID;
    using typename Base::TreePathT;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::BranchNodeEntry;

    void check(const CheckResultConsumerFn& fn) {
        self().ctr_check_tree(fn);
    }

    void ctr_check_it(const CheckResultConsumerFn& fn)
    {
        self().ctr_check_tree(fn);
    }

    void ctr_check_path(const TreePathT& path, size_t level = 0) const
    {
//        //auto& self = this->self();
//        for (size_t ll = level; ll < path.size(); ll++)
//        {
//            if (!path[ll]) {
//                MEMORIA_MAKE_GENERIC_ERROR("Null path element: {}", ll).do_throw();
//            }
//            else if (path[ll]->level() != ll) {
//                MEMORIA_MAKE_GENERIC_ERROR("Invalid node level: {} {} {}", path[ll]->id(), path[ll]->level(), ll).do_throw();
//            }

//            if (ll == 0 && !path[ll]->is_leaf()) {
//                MEMORIA_MAKE_GENERIC_ERROR("Lowest node is not leaf: {} {} {}", path[ll]->id(), path[ll]->level(), ll).do_throw();
//            }

//            if (ll == path.size() - 1 && !path[ll]->is_root()) {
//                MEMORIA_MAKE_GENERIC_ERROR("Topmost node is not root: {} {} {}", path[ll]->id(), path[ll]->level(), ll).do_throw();
//            }

//            //BlockID child_id = path[ll - 1]->id();

//            // FIXME: handle returned index!
//            //self.ctr_get_child_idx(path[ll], child_id);
//        }
    }

    void ctr_check_same_paths(
            const TreePathT& left_path,
            const TreePathT& right_path,
            size_t level = 0
    ) const
    {
        if (left_path.size() == right_path.size())
        {
            for (size_t ll = level; ll < left_path.size(); ll++)
            {
                if (left_path[ll] != right_path[ll]) {
                    MEMORIA_MAKE_GENERIC_ERROR(
                                "Path nodes are not equal at the level {} :: {} {}",
                                ll,
                                left_path[ll]->id(),
                                right_path[ll]->id()
                    ).do_throw();
                }
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Path sizes are different: {} {}",
                        left_path.size(),
                        right_path.size()
            ).do_throw();
        }
    }

public:
    void ctr_check_tree(const CheckResultConsumerFn& fn) const ;

    MEMORIA_V1_DECLARE_NODE_FN(CheckContentFn, check);
    void ctr_check_content(const TreeNodeConstPtr& node, const CheckResultConsumerFn& fn) const
    {
        self().node_dispatcher().dispatch(node, CheckContentFn(), fn);
    }

private:
    void ctr_check_tree_structure(
            const TreeNodeConstPtr& parent,
            int32_t parent_idx,
            int32_t level,
            const TreeNodeConstPtr& node,
            const CheckResultConsumerFn& fn
    ) const ;


    template <typename Node1, typename Node2>
    void ctr_check_typed_node_content(
            Node1&& node,
            Node2&& parent,
            int32_t parent_idx,
            const CheckResultConsumerFn& fn
    ) const;

    MEMORIA_V1_CONST_FN_WRAPPER(CheckTypedNodeContentFn, ctr_check_typed_node_content);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ChecksName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::ctr_check_tree(const CheckResultConsumerFn& fn) const
{
    auto& self = this->self();

    auto root = self.ctr_get_root_node();
    if (root)
    {
        self.ctr_check_tree_structure(TreeNodeConstPtr(), 0, root->level(), root, fn);
    }
    else {
        fn(CheckSeverity::ERROR, make_string_document("No root node is specified for container {}", self.name()));
    }
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::ctr_check_tree_structure(
        const TreeNodeConstPtr& parent,
        int32_t parent_idx,
        int32_t level,
        const TreeNodeConstPtr& node,
        const CheckResultConsumerFn& fn
) const
{
    auto& self = the_self();

    bool allocated = self.store().is_allocated(node->id());
    if (!allocated) {
        fn(CheckSeverity::ERROR, make_string_document("Node {} is not marked as allocated in the store", node->id()));
    }

    if (node->level() != level) {
        fn(CheckSeverity::ERROR, make_string_document("Invalid tree node level for node {}. Expected: {}, actual: {}", node->id(), level, node->level()));
    }

    self.store().check_storage(node, fn);
    self.ctr_check_content(node, fn);

    if (!node->is_root())
    {
        self.tree_dispatcher().dispatchTree(parent, node, CheckTypedNodeContentFn(self), parent_idx, fn);

        if (!node->is_leaf())
        {
            auto children = self.ctr_get_node_size(node, 0);
            if (children == 0 && !node->is_root())
            {
                fn(CheckSeverity::ERROR, make_string_document("children == 0 for non-root node, node = {}", self.ctr_describe_node(node)));
            }
        }
    }

    if (!node->is_leaf())
    {
        auto children = self.ctr_get_node_size(node, 0);

        for (int32_t c = 0; c < children; c++)
        {
            auto child_id = self.ctr_get_child_id(node, c);
            auto child = self.ctr_get_node_child(node, c);

            int child_idx = self.ctr_get_child_idx(node, child_id);

            if (child_idx != c)
            {
                fn(CheckSeverity::ERROR, make_string_document("child.idx != child_idx :: {} {} {} {}", node->id(), child->id(), c, child_idx));
            }

            if (child->id() != child_id)
            {
                fn(CheckSeverity::ERROR, make_string_document("child.id != child_id :: {} {} {}", child->id(), child->id(), child_id));
            }

            self.ctr_check_tree_structure(node, c, level - 1, child, fn);
        }
    }
    else {
        if (node->level() != 0)
        {
            fn(CheckSeverity::ERROR, make_string_document("Node {} marked as leaf, but it's level is not zero: ", node->id(), node->level()));
        }
    }
}

M_PARAMS
template <typename Node1, typename Node2>
void M_TYPE::ctr_check_typed_node_content(
        Node1&& parent,
        Node2&& node,
        int32_t parent_idx,
        const CheckResultConsumerFn& fn
) const
{
    BranchNodeEntry sums;
    node.max(sums);

    auto keys = parent.keysAt(parent_idx);
    if (sums != keys)
    {

        fn(CheckSeverity::ERROR,
                    make_string_document(
                        "Invalid parent-child nodes chain :: {} {} for node.id={} parent.id={}, parent_idx={}",
                        (SBuf() << sums).str(),
                        (SBuf() << keys).str(),
                        node.node()->id(),
                        parent.node()->id(),
                        parent_idx
                    )
         );

    }
}

#undef M_TYPE
#undef M_PARAMS

}
