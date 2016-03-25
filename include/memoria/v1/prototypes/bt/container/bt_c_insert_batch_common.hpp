
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>
#include <algorithm>

namespace memoria {
namespace v1 {

using namespace v1::bt;
using namespace v1::core;

using namespace std;

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bt::InsertBatchCommonName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    class Checkpoint {
        NodeBaseG head_;
        Int size_;
    public:
        Checkpoint(NodeBaseG head, Int size): head_(head), size_(size) {}

        NodeBaseG head() const {return head_;};
        Int size() const {return size_;};
    };


    struct ILeafProvider {
        virtual NodeBaseG get_leaf()    = 0;

        virtual Checkpoint checkpoint() = 0;

        virtual void rollback(const Checkpoint& chekpoint) = 0;

        virtual CtrSizeT size() const       = 0;
    };



    void updateChildIndexes(NodeBaseG& node, Int start)
    {
        auto& self = this->self();
        Int size = self.getBranchNodeSize(node);

        if (start < size)
        {
            self.forAllIDs(node, start, size, [&, this](const ID& id, Int parent_idx)
            {
                auto& self = this->self();
                NodeBaseG child = self.allocator().getPageForUpdate(id, self.master_name());

                child->parent_idx() = parent_idx;
            });
        }
    }

    void remove_branch_nodes(ID node_id)
    {
        auto& self = this->self();

        NodeBaseG node = self.allocator().getPage(node_id, self.master_name());

        if (node->level() > 0)
        {
            self.forAllIDs(node, [&, this](const ID& id, Int idx)
            {
                auto& self = this->self();
                self.remove_branch_nodes(id);
            });

            self.allocator().removePage(node->id(), self.master_name());
        }
    }

    class InsertBatchResult {
        Int idx_;
        CtrSizeT subtree_size_;
    public:
        InsertBatchResult(Int idx, CtrSizeT size): idx_(idx), subtree_size_(size) {}

        Int idx() const {return idx_;}
        CtrSizeT subtree_size() const {return subtree_size_;}
    };


    NodeBaseG BuildSubtree(ILeafProvider& provider, Int level)
    {
        auto& self = this->self();

        if (provider.size() > 0)
        {
            if (level >= 1)
            {
                NodeBaseG node = self.createNode1(level, false, false);

                self.layoutNonLeafNode(node, 0xFF);

                self.insertSubtree(node, 0, provider, [this, level, &provider]() -> NodeBaseG {
                    auto& self = this->self();
                    return self.BuildSubtree(provider, level - 1);
                }, false);

                return node;
            }
            else {
                return provider.get_leaf();
            }
        }
        else {
            return NodeBaseG();
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

        virtual NodeBaseG get_leaf()
        {
            if (head_.isSet())
            {
                auto node = head_;
                head_ = ctr_.allocator().getPage(head_->next_leaf_id(), ctr_.master_name());
                size_--;
                return node;
            }
            else {
                throw v1::BoundsException(MA_SRC, "Leaf List is empty");
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


    void updateChildren(const NodeBaseG& node);
    void updateChildren(const NodeBaseG& node, Int start);
    void updateChildren(const NodeBaseG& node, Int start, Int end);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(IsEmptyFn, isEmpty, bool);
    bool isEmpty(const NodeBaseG& node) {
        return NodeDispatcher::dispatch(node, IsEmptyFn());
    }

private:
    void updateChildrenInternal(const NodeBaseG& node, Int start, Int end);
public:


    NodeBaseG createNextLeaf(NodeBaseG& left_node);

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bt::InsertBatchCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::createNextLeaf(NodeBaseG& left_node)
{
    auto& self = this->self();

    if (left_node->is_root())
    {
        self.newRootP(left_node);
    }
    else {
        self.updatePageG(left_node);
    }

    NodeBaseG left_parent = self.getNodeParentForUpdate(left_node);

    NodeBaseG other  = self.createNode1(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

    other->next_leaf_id().clear();

    ListLeafProvider provider(self, other, 1);

    self.insert_subtree(left_parent, left_node->parent_idx() + 1, provider);

    return other;
}







M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node)
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        self.updateChildrenInternal(node, 0, self.getBranchNodeSize(node));
    }
}

M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node, Int start)
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        self.updateChildrenInternal(node, start, self.getBranchNodeSize(node));
    }
}

M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node, Int start, Int end)
{
    if (!node->is_leaf())
    {
        auto& self = this->self();
        self.updateChildrenInternal(node, start, end);
    }
}


M_PARAMS
void M_TYPE::updateChildrenInternal(const NodeBaseG& node, Int start, Int end)
{
    auto& self = this->self();

    ID node_id = node->id();

    self.forAllIDs(node, start, end, [&self, &node_id](const ID& id, Int idx)
    {
        NodeBaseG child = self.allocator().getPageForUpdate(id, self.master_name());

        child->parent_id()  = node_id;
        child->parent_idx() = idx;
    });
}


#undef M_TYPE
#undef M_PARAMS

}}