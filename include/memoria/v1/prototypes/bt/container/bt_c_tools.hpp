
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
#include <memoria/v1/core/strings/string.hpp>

#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <iostream>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ToolsName)
public:
    using Types = typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Allocator::BlockG                                     BlockG;

    using typename Base::BlockID;
    using typename Base::BlockType;

    using Profile = typename Types::Profile;

    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

public:
    NodeBaseG getRoot() const
    {
        auto& self = this->self();
        return self.allocator().getBlock(self.root());
    }

    NodeBaseG getRootForUpdate() const
    {
        auto& self = this->self();
        return self.allocator().getBlockForUpdate(self.root());
    }

    MEMORIA_V1_DECLARE_NODE_FN(DumpBlockSizesFn, dumpBlockSizes);
    void dumpBlockSizes(const NodeBaseG& node) const
    {
        self().leaf_dispatcher().dispatch(node, DumpBlockSizesFn());
    }


    MEMORIA_V1_DECLARE_NODE_FN(MaxFn, max);
    BranchNodeEntry max(const NodeBaseG& node) const
    {
        BranchNodeEntry entry;
        self().node_dispatcher().dispatch(node, MaxFn(), entry);
        return entry;
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetSizesFn, sizes, Position);
    Position getNodeSizes(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, GetSizesFn());
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetSizeFn, size, int32_t);
    int32_t getNodeSize(const NodeBaseG& node, int32_t stream) const
    {
        return self().node_dispatcher().dispatch(node, GetSizeFn(), stream);
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(CheckCapacitiesFn, checkCapacities, bool);
    bool checkCapacities(const NodeBaseG& node, const Position& sizes) const
    {
        return self().node_dispatcher().dispatch(node, CheckCapacitiesFn(), sizes);
    }

    void dump(const NodeBaseG& block, std::ostream& out = std::cout) const
    {
        if (block)
        {
            TextBlockDumper dumper(out);

            ProfileMetadata<Profile>::local()
                    ->get_block_operations(block->ctr_type_hash(), block->block_type_hash())
                    ->generateDataEvents(block.block()->as_header(), DataEventsParams(), &dumper);

            out<<std::endl;
            out<<std::endl;
        }
        else {
            out << "NULL" << std::endl;
        }
    }

    void dumpPath(NodeBaseG node, std::ostream& out = std::cout, int32_t depth = 100) const
    {
        auto& self = this->self();

        out << "Path:" << std::endl;

        self.dump(node, out);

        while (!node->is_root() && node->level() < depth)
        {
            node = self.getNodeParent(node);
            self.dump(node, out);
        }
    }


protected:

    bool isTheSameNode(const NodeBaseG& node1, const NodeBaseG& node2) const
    {
        return node1->id() == node2->id();
    }

    void root2Node(NodeBaseG& node) const
    {
        self().updateBlockG(node);

        node->set_root(false);

        node->clearMetadata();
    }

    void node2Root(NodeBaseG& node, const Metadata& meta) const
    {
        self().updateBlockG(node);

        node->set_root(true);

        node->parent_id().clear();
        node->parent_idx() = 0;

        node->setMetadata(meta);
    }

    void copyRootMetadata(NodeBaseG& src, NodeBaseG& tgt) const
    {
        self().updateBlockG(tgt);
        tgt->setMetadata(src->root_metadata());
    }

    bool canConvertToRoot(const NodeBaseG& node) const
    {
        return node->canConvertToRoot();
    }



    template <typename Node>
    NodeBaseG getChildFn(const Node* node, int32_t idx) const
    {
        auto& self = this->self();
        return self.allocator().getBlock(node->value(idx));
    }

    template <typename Node>
    NodeBaseG getLastChildFn(const Node* node, int32_t idx) const
    {
        auto& self = this->self();
        return self.allocator().getBlock(node->value(node->size() - 1));
    }


    template <typename Node>
    NodeBaseG getChildForUpdateFn(const Node* node, int32_t idx) const
    {
        auto& self = this->self();
        return self.allocator().getBlockForUpdate(node->value(idx));
    }


    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetChildFn, getChildFn, NodeBaseG);
    NodeBaseG getChild(const NodeBaseG& node, int32_t idx) const
    {
        NodeBaseG result = self().branch_dispatcher().dispatch(node, GetChildFn(self()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            MMA1_THROW(NullPointerException()) << WhatCInfo("Child must not be NULL");
        }
    }

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetLastChildFn, getLastChildFn, NodeBaseG);
    NodeBaseG getLastChild(const NodeBaseG& node, int32_t idx) const
    {
        NodeBaseG result = self().branch_dispatcher().dispatch(node, GetLastChildFn(self()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            MMA1_THROW(NullPointerException()) << WhatCInfo("Child must not be NULL");
        }
    }



    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetChildForUpdateFn, getChildForUpdateFn, NodeBaseG);
    NodeBaseG getChildForUpdate(const NodeBaseG& node, int32_t idx) const
    {
        NodeBaseG result = self().branch_dispatcher().dispatch(node, GetChildForUpdateFn(self()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            MMA1_THROW(NullPointerException()) << WhatCInfo("Child must not be NULL");
        }
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(NodeStreamSizesFn, size_sums, Position);
    Position node_stream_sizes(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, NodeStreamSizesFn());
    }



    MEMORIA_V1_DECLARE_NODE_FN(SumsFn, sums);
    void sums(const NodeBaseG& node, BranchNodeEntry& sums) const
    {
        self().node_dispatcher().dispatch(node, SumsFn(), sums);
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SumsRtnFn, sums, BranchNodeEntry);
    BranchNodeEntry sums(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, SumsRtnFn());
    }

    void sums(const NodeBaseG& node, int32_t start, int32_t end, BranchNodeEntry& sums) const
    {
        self().node_dispatcher().dispatch(node, SumsFn(), start, end, sums);
    }

    BranchNodeEntry sums(const NodeBaseG& node, int32_t start, int32_t end) const
    {
        BranchNodeEntry sums;
        self().node_dispatcher().dispatch(node, SumsFn(), start, end, sums);
        return sums;
    }

    void sums(const NodeBaseG& node, const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        self().node_dispatcher().dispatch(node, SumsFn(), start, end, sums);
    }

    BranchNodeEntry sums(const NodeBaseG& node, const Position& start, const Position& end) const
    {
        BranchNodeEntry sums;
        self().node_dispatcher().dispatch(node, SumsFn(), start, end, sums);
        return sums;
    }



    template <typename Path>
    struct LeafSumsFn {
        template <typename Node, typename... Args>
        auto treeNode(const Node* node, Args&&... args) {
            return node->template leaf_sums<Path>(std::forward<Args>(args)...);
        }
    };


    struct LeafSizesFn {
        template <typename Node, typename... Args>
        auto treeNode(const Node* node, Args&&... args) {
            return node->sizes();
        }
    };

    template <typename Path, typename... Args>
    auto leaf_sums(const NodeBaseG& node, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(node, LeafSumsFn<Path>(), std::forward<Args>(args)...);
    }


    auto leaf_sizes(const NodeBaseG& node) const
    {
        return self().leaf_dispatcher().dispatch(node, LeafSizesFn());
    }




    MEMORIA_V1_DECLARE_NODE_FN(SetKeysFn, setKeys);
    void setBranchKeys(NodeBaseG& node, int32_t idx, const BranchNodeEntry& keys) const
    {
        self().updateBlockG(node);
        self().branch_dispatcher().dispatch(node, SetKeysFn(), idx, keys);
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetINodeDataFn, value, BlockID);
    BlockID getChildID(const NodeBaseG& node, int32_t idx) const
    {
        return self().branch_dispatcher().dispatch(node, GetINodeDataFn(), idx);
    }


public:

    NodeBaseG getNextNodeP(NodeBaseG& node) const;
    NodeBaseG getPrevNodeP(NodeBaseG& node) const;


    MEMORIA_V1_DECLARE_NODE_FN(LayoutNodeFn, layout);
    void layoutBranchNode(NodeBaseG& node, uint64_t active_streams) const
    {
        self().branch_dispatcher().dispatch(node, LayoutNodeFn(), active_streams);
    }

    void layoutLeafNode(NodeBaseG& node, const Position& sizes) const
    {
        self().leaf_dispatcher().dispatch(node, LayoutNodeFn(), sizes);
    }


protected:
    MEMORIA_V1_DECLARE_NODE_FN_RTN(GetActiveStreamsFn, active_streams, uint64_t);
    uint64_t getActiveStreams(const NodeBaseG& node) const
    {
        return self().node_dispatcher().dispatch(node, GetActiveStreamsFn());
    }

    void initLeaf(NodeBaseG& node) const {}

    MEMORIA_V1_DECLARE_NODE_FN_RTN(IsNodeEmpty, is_empty, bool);
    bool isNodeEmpty(const NodeBaseG& node)
    {
        return self().node_dispatcher().dispatch(node, IsNodeEmpty());
    }


    MEMORIA_V1_DECLARE_NODE_FN(ForAllIDsFn, forAllValues);
    void forAllIDs(const NodeBaseG& node, int32_t start, int32_t end, std::function<void (const BlockID&, int32_t)> fn) const
    {
        self().branch_dispatcher().dispatch(node, ForAllIDsFn(), start, end, fn);
    }

    void forAllIDs(const NodeBaseG& node, int32_t start, std::function<void (const BlockID&, int32_t)> fn) const
    {
        self().branch_dispatcher().dispatch(node, ForAllIDsFn(), start, fn);
    }

    void forAllIDs(const NodeBaseG& node, std::function<void (const BlockID&, int32_t)> fn) const
    {
        self().branch_dispatcher().dispatch(node, ForAllIDsFn(), fn);
    }


    struct SetChildIDFn {
        template <typename T>
        void treeNode(bt::BranchNode<T>* node, int child_idx, const BlockID& child_id) const
        {
            node->value(child_idx) = child_id;
        }
    };

    void setChildId(NodeBaseG& node, int32_t child_idx, const BlockID& child_id) const
    {
        self().updateBlockG(node);
        self().branch_dispatcher().dispatch(node, SetChildIDFn(), child_idx, child_id);
    }



    struct GetBranchNodeChildernCount {
        template <typename T, typename... Args>
        int32_t treeNode(const bt::BranchNode<T>* node, Args&&... args) const
        {
            return node->size();
        }
    };

    template <int32_t Stream>
    struct GetLeafNodeStreamSize {
        template <typename T, typename... Args>
        int32_t treeNode(const bt::LeafNode<T>* node, Args&&... args) const
        {
            return node->template streamSize<Stream>();
        }
    };

    struct GetLeafNodeStreamSizes {
        template <typename T, typename... Args>
        Position treeNode(const bt::LeafNode<T>* node, Args&&... args) const
        {
            return node->sizes();
        }
    };

    struct GetLeafNodeStreamSizesStatic {
        template <typename T, typename... Args>
        Position treeNode(const bt::LeafNode<T>* node, Args&&... args) const
        {
            return bt::LeafNode<T>::sizes(std::forward<Args>(args)...);
        }
    };


    int32_t getNodeChildrenCount(const NodeBaseG& node) const
    {
        return self().branch_dispatcher().dispatch(node, GetBranchNodeChildernCount());
    }



    int32_t getBranchNodeSize(const NodeBaseG& node) const
    {
        return self().branch_dispatcher().dispatch(node, GetSizeFn());
    }

public:

    template <int32_t StreamIdx>
    int32_t getLeafStreamSize(const NodeBaseG& node) const
    {
        return self().leaf_dispatcher().dispatch(node, GetLeafNodeStreamSize<StreamIdx>());
    }

    Position getLeafStreamSizes(const NodeBaseG& node) const
    {
        return self().leaf_dispatcher().dispatch(node, GetLeafNodeStreamSizes());
    }


    Position getStreamSizes(const BranchNodeEntry& sums) const
    {
        return self().leaf_dispatcher().template dispatch<bt::LeafNode>(true, GetLeafNodeStreamSizesStatic(), sums);
    }

    struct CreateIOVectorViewFn
    {
        template <typename T, typename... Args>
        std::unique_ptr<io::IOVector> treeNode(bt::LeafNode<T>* node) const
        {
            return node->create_iovector_view();
        }
    };

    std::unique_ptr<io::IOVector> create_iovector_view(NodeBaseG& node)
    {
        return self().leaf_dispatcher().dispatch(node, CreateIOVectorViewFn());
    }


    struct ConfigureIOVectorViewFn
    {
        template <typename T, typename... Args>
        void treeNode(bt::LeafNode<T>* node, io::IOVector& io_vector) const
        {
            return node->configure_iovector_view(io_vector);
        }
    };

    void configure_iovector_view(NodeBaseG& node, io::IOVector& io_vector)
    {
        return self().leaf_dispatcher().dispatch(node, ConfigureIOVectorViewFn(), io_vector);
    }

protected:

    template <typename SubstreamPath>
    struct GetPackedStructFn {
        template <typename T>
        auto treeNode(const bt::LeafNode<T>* node) const
        {
            return node->template substream<SubstreamPath>();
        }

        template <typename T>
        auto treeNode(bt::LeafNode<T>* node) const
        {
            return node->template substream<SubstreamPath>();
        }
    };



    template <typename SubstreamPath>
    const auto* getPackedStruct(const NodeBaseG& leaf) const
    {
        return self().leaf_dispatcher().dispatch(leaf, GetPackedStructFn<SubstreamPath>());
    }

    template <typename SubstreamPath>
    auto* getPackedStruct(NodeBaseG& leaf)
    {
        return self().leaf_dispatcher().dispatch(leaf, GetPackedStructFn<SubstreamPath>());
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS






M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::getNextNodeP(NodeBaseG& node) const
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParent(node);

        int32_t size = self.getNodeSize(parent, 0);

        int32_t parent_idx = node->parent_idx();

        if (parent_idx < size - 1)
        {
            return self.getChild(parent, parent_idx + 1);
        }
        else {
            NodeBaseG target_parent = getNextNodeP(parent);

            if (target_parent.isSet())
            {
                return self.getChild(target_parent, 0);
            }
            else {
                return target_parent;
            }
        }
    }
    else {
        return NodeBaseG();
    }
}


M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::getPrevNodeP(NodeBaseG& node) const
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParent(node);

        int32_t parent_idx = node->parent_idx();

        if (parent_idx > 0)
        {
            return self.getChild(parent, parent_idx - 1);
        }
        else {
            NodeBaseG target_parent = getPrevNodeP(parent);

            if (target_parent.isSet())
            {
                int32_t node_size = self.getNodeSize(target_parent, 0);
                return self.getChild(target_parent, node_size - 1);
            }
            else {
                return target_parent;
            }
        }
    }
    else {
        return NodeBaseG();
    }
}






#undef M_TYPE
#undef M_PARAMS

}}
