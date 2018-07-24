
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

#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ChecksName)
private:
    
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;
    using TreeDispatcher    = typename Types::Pages::TreeDispatcher;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;

    bool check(void *data) const
    {
        return self().checkTree();
    }



    void checkIt()
    {
        auto& self = this->self();

        self.logger().level() = Logger::_ERROR;

        if (self.checkTree())
        {
            throw Exception(MA_SRC, SBuf()<<"Container "<<self.name()<<" ("<<self.type_name_str()<<") check failed");
        }
    }

public:
    bool checkTree() const;

    MEMORIA_V1_DECLARE_NODE_FN(CheckContentFn, check);
    bool checkContent(const NodeBaseG& node) const
    {
        try {
            NodeDispatcher::dispatch(node, CheckContentFn());
            return false;
        }
        catch (Exception& ex)
        {
            self().dump(node);

            MMA1_ERROR(self(), "Node content check failed", ex.message());
            return true;
        }
    }



private:
    void checkTreeStructure(const NodeBaseG& parent, int32_t parent_idx, const NodeBaseG& node, bool &errors) const;


    template <typename Node1, typename Node2>
    bool checkTypedNodeContent(const Node1 *node, const Node2 *parent, int32_t parent_idx) const;

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(CheckTypedNodeContentFn, checkTypedNodeContent, bool);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ChecksName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::checkTree() const
{
    auto& self = this->self();

    NodeBaseG root = self.getRoot();
    if (root)
    {
        bool errors = false;
        self.checkTreeStructure(NodeBaseG(), 0, root, errors);
        return errors;
    }
    else {
        MMA1_ERROR(self, "No root node for container");
        return true;
    }
}






// ------------------------ PRIVATE API -----------------------------


M_PARAMS
void M_TYPE::checkTreeStructure(const NodeBaseG& parent, int32_t parent_idx, const NodeBaseG& node, bool &errors) const
{
    auto& self = this->self();

    errors = self.checkContent(node) || errors;

    if (!node->is_root())
    {
        errors = TreeDispatcher::dispatchTree(parent, node, CheckTypedNodeContentFn(self), parent_idx) || errors;

        if (!node->is_leaf())
        {
            int32_t children = self.getNodeSize(node, 0);

            if (children == 0 && !node->is_root())
            {
                errors = true;
                MMA1_ERROR(self, "children == 0 for non-root node", node->id());
                self.dump(node);
            }
        }
    }

    if (!node->is_leaf())
    {
        int32_t children = self.getNodeSize(node, 0);

        for (int32_t c = 0; c < children; c++)
        {
            ID child_id = self.getChildID(node, c);

            NodeBaseG child = self.getChild(node, c);

            if (child->id() != child_id)
            {
                errors = true;
                MMA1_ERROR(self, "child.id != child_id", child->id(), child->id(), child_id);
            }

            if (child->parent_idx() != c)
            {
                errors = true;
                MMA1_ERROR(self, "child.parent_idx != idx", child->parent_idx(), c, node->id(), child->id());
                std::cout << "parent_idx: " << child->parent_idx() << " " << c << std::endl;
            }

            if (child->parent_id() != node->id())
            {
                errors = true;
                MMA1_ERROR(self, "child.parent_id != node.id", child->parent_id(), node->id());
                std::cout << "parent_idx: " << child->parent_id() << " " << node->id() << std::endl;
            }

            self.checkTreeStructure(node, c, child, errors);
        }
    }
}

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::checkTypedNodeContent(const Node1 *parent, const Node2* node, int32_t parent_idx) const
{
    bool errors = false;

    BranchNodeEntry sums;
    node->max(sums);

    BranchNodeEntry keys = parent->keysAt(parent_idx);

    if (sums != keys)
    {
        MMA1_ERROR(
                self(),
                "Invalid parent-child nodes chain",
                (SBuf()<<sums).str(),
                (SBuf()<<keys).str(),
                "for node.id=",
                node->id(),
                "parent.id=",
                parent->id(),
                "parent_idx",
                parent_idx
        );

        errors = true;
    }


    return errors;
}

#undef M_TYPE
#undef M_PARAMS

}}
