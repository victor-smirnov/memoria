
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_TOOLS_HPP

#include <memoria/metadata/tools.hpp>
#include <memoria/core/tools/strings.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::bt;



MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::ToolsName)



    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Allocator::PageG                                     PageG;

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    static const Int Streams                                                    = Types::Streams;

    bool isTheSameNode(const NodeBaseG& node1, const NodeBaseG& node2) const
    {
        return node1->id() == node2->id();
    }

    void root2Node(NodeBaseG& node) const
    {
        self().updatePageG(node);

        node->set_root(false);

        node->clearMetadata();
    }

    void node2Root(NodeBaseG& node, const Metadata& meta) const
    {
        self().updatePageG(node);

        node->set_root(true);

        node->parent_id().clear();
        node->parent_idx() = 0;

        node->setMetadata(meta);
    }

    void copyRootMetadata(NodeBaseG& src, NodeBaseG& tgt) const
    {
        self().updatePageG(tgt);
        tgt->setMetadata(src->root_metadata());
    }

    bool canConvertToRoot(const NodeBaseG& node) const
    {
        return node->canConvertToRoot();
    }



    template <typename Node>
    NodeBaseG getChildFn(const Node* node, Int idx) const
    {
        auto& self = this->self();
        return self.allocator().getPage(node->value(idx), self.master_name());
    }


    template <typename Node>
    NodeBaseG getChildForUpdateFn(const Node* node, Int idx) const
    {
        auto& self = this->self();
        return self.allocator().getPageForUpdate(node->value(idx), self.master_name());
    }


    MEMORIA_CONST_FN_WRAPPER_RTN(GetChildFn, getChildFn, NodeBaseG);
    NodeBaseG getChild(const NodeBaseG& node, Int idx) const
    {
        NodeBaseG result = BranchDispatcher::dispatch(node, GetChildFn(me()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Child must not be NULL");
        }
    }



    MEMORIA_CONST_FN_WRAPPER_RTN(GetChildForUpdateFn, getChildForUpdateFn, NodeBaseG);
    NodeBaseG getChildForUpdate(const NodeBaseG& node, Int idx) const
    {
        NodeBaseG result = BranchDispatcher::dispatch(node, GetChildForUpdateFn(me()), idx);

        if (result.isSet())
        {
            return result;
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Child must not be NULL");
        }
    }


    NodeBaseG getNodeParent(const NodeBaseG& node) const
    {
        auto& self = this->self();
        return self.allocator().getPage(node->parent_id(), self.master_name());
    }

    NodeBaseG getNodeParentForUpdate(const NodeBaseG& node) const
    {
        auto& self = this->self();
        return self.allocator().getPageForUpdate(node->parent_id(), self.master_name());
    }


    MEMORIA_DECLARE_NODE_FN_RTN(NodeStreamSizesFn, size_sums, Position);
    Position node_stream_sizes(const NodeBaseG& node) const
    {
        return NodeDispatcher::dispatch(node, NodeStreamSizesFn());
    }



    MEMORIA_DECLARE_NODE_FN(SumsFn, sums);
    void sums(const NodeBaseG& node, BranchNodeEntry& sums) const
    {
        NodeDispatcher::dispatch(node, SumsFn(), sums);
    }

    MEMORIA_DECLARE_NODE_FN_RTN(SumsRtnFn, sums, BranchNodeEntry);
    BranchNodeEntry sums(const NodeBaseG& node) const
    {
        return NodeDispatcher::dispatch(node, SumsRtnFn());
    }

    void sums(const NodeBaseG& node, Int start, Int end, BranchNodeEntry& sums) const
    {
        NodeDispatcher::dispatch(node, SumsFn(), start, end, sums);
    }

    BranchNodeEntry sums(const NodeBaseG& node, Int start, Int end) const
    {
        BranchNodeEntry sums;
        NodeDispatcher::dispatch(node, SumsFn(), start, end, sums);
        return sums;
    }

    void sums(const NodeBaseG& node, const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        NodeDispatcher::dispatch(node, SumsFn(), start, end, sums);
    }

    BranchNodeEntry sums(const NodeBaseG& node, const Position& start, const Position& end) const
    {
        BranchNodeEntry sums;
        NodeDispatcher::dispatch(node, SumsFn(), start, end, sums);
        return sums;
    }

    MEMORIA_DECLARE_NODE_FN(MaxFn, max);
    BranchNodeEntry max(const NodeBaseG& node) const
    {
    	BranchNodeEntry entry;
    	NodeDispatcher::dispatch(node, MaxFn(), entry);
    	return entry;
    }

    template <typename Path>
    struct LeafSumsFn {
    	template <typename Node, typename... Args>
    	auto treeNode(const Node* node, Args&&... args) {
    		return node->template leaf_sums<Path>(std::forward<Args>(args)...);
    	}
    };

    template <typename Path, typename... Args>
    auto leaf_sums(const NodeBaseG& node, Args&&... args) const
    {
    	return LeafDispatcher::dispatch(node, LeafSumsFn<Path>(), std::forward<Args>(args)...);
    }

    NodeBaseG getRoot() const
    {
        auto& self = this->self();
        return self.allocator().getPage(self.root(), self.master_name());
    }

    NodeBaseG getRootForUpdate() const
    {
        auto& self = this->self();
        return self.allocator().getPageForUpdate(self.root(), self.master_name());
    }


    MEMORIA_DECLARE_NODE_FN(SetKeysFn, setKeys);


    void setBranchKeys(NodeBaseG& node, Int idx, const BranchNodeEntry& keys) const
    {
        self().updatePageG(node);
        BranchDispatcher::dispatch(node, SetKeysFn(), idx, keys);
    }


    MEMORIA_DECLARE_NODE_FN_RTN(GetINodeDataFn, value, ID);
    ID getChildID(const NodeBaseG& node, Int idx) const
    {
        return BranchDispatcher::dispatch(node, GetINodeDataFn(), idx);
    }


    void dump(const NodeBaseG& page, std::ostream& out = std::cout) const
    {
        if (page)
        {
            PageWrapper<const Page> pw(page);
            PageMetadata* meta = me()->getMetadata()->getPageMetadata(pw.getContainerHash(), pw.getPageTypeHash());
            memoria::vapi::dumpPage(meta, &pw, out);
            out<<std::endl;
            out<<std::endl;
        }
        else {
            out<<"NULL"<<std::endl;
        }
    }

    void dumpPath(NodeBaseG node, std::ostream& out = std::cout, Int depth = 100) const
    {
        auto& self = this->self();

        out<<"Path:"<<std::endl;

        self.dump(node, out);

        while (!node->is_root() && node->level() < depth)
        {
            node = self.getNodeParent(node);
            self.dump(node, out);
        }
    }


    NodeBaseG getNextNodeP(NodeBaseG& node) const;
    NodeBaseG getPrevNodeP(NodeBaseG& node) const;


    MEMORIA_DECLARE_NODE_FN_RTN(CheckCapacitiesFn, checkCapacities, bool);
    bool checkCapacities(const NodeBaseG& node, const Position& sizes) const
    {
        return NodeDispatcher::dispatch(node, CheckCapacitiesFn(), sizes);
    }





    MEMORIA_DECLARE_NODE_FN(LayoutNodeFn, layout);
    void layoutBranchNode(NodeBaseG& node, UBigInt active_streams) const
    {
        BranchDispatcher::dispatch(node, LayoutNodeFn(), active_streams);
    }

    void layoutLeafNode(NodeBaseG& node, const Position& sizes) const
    {
        LeafDispatcher::dispatch(node, LayoutNodeFn(), sizes);
    }

    MEMORIA_DECLARE_NODE_FN_RTN(GetActiveStreamsFn, active_streams, UBigInt);
    UBigInt getActiveStreams(const NodeBaseG& node) const
    {
        return NodeDispatcher::dispatch(node, GetActiveStreamsFn());
    }

    void initLeaf(NodeBaseG& node) const {}

    MEMORIA_DECLARE_NODE_FN_RTN(IsNodeEmpty, is_empty, bool);
    bool isNodeEmpty(const NodeBaseG& node)
    {
        return NodeDispatcher::dispatch(node, IsNodeEmpty());
    }


    MEMORIA_DECLARE_NODE_FN(ForAllIDsFn, forAllValues);
    void forAllIDs(const NodeBaseG& node, Int start, Int end, std::function<void (const ID&, Int)> fn) const
    {
        BranchDispatcher::dispatch(node, ForAllIDsFn(), start, end, fn);
    }

    void forAllIDs(const NodeBaseG& node, Int start, std::function<void (const ID&, Int)> fn) const
    {
    	BranchDispatcher::dispatch(node, ForAllIDsFn(), start, fn);
    }

    void forAllIDs(const NodeBaseG& node, std::function<void (const ID&, Int)> fn) const
    {
    	BranchDispatcher::dispatch(node, ForAllIDsFn(), fn);
    }


    struct GetBranchNodeChildernCount {
        template <typename T, typename... Args>
        Int treeNode(const BranchNode<T>* node, Args&&... args) const
        {
            return node->size();
        }
    };

    template <Int Stream>
    struct GetLeafNodeStreamSize {
    	template <typename T, typename... Args>
    	Int treeNode(const LeafNode<T>* node, Args&&... args) const
    	{
    		return node->template streamSize<Stream>();
    	}
    };

    struct GetLeafNodeStreamSizes {
    	template <typename T, typename... Args>
    	Position treeNode(const LeafNode<T>* node, Args&&... args) const
    	{
    		return node->sizes();
    	}
    };

    struct GetLeafNodeStreamSizesStatic {
    	template <typename T, typename... Args>
    	Position treeNode(const LeafNode<T>* node, Args&&... args) const
    	{
    		return LeafNode<T>::sizes(std::forward<Args>(args)...);
    	}
    };


    Int getNodeChildrenCount(const NodeBaseG& node) const
    {
    	return BranchDispatcher::dispatch(node, GetBranchNodeChildernCount());
    }


    MEMORIA_DECLARE_NODE_FN_RTN(GetSizesFn, sizes, Position);
    Position getNodeSizes(const NodeBaseG& node) const
    {
        return NodeDispatcher::dispatch(node, GetSizesFn());
    }

    MEMORIA_DECLARE_NODE_FN_RTN(GetSizeFn, size, Int);
    Int getNodeSize(const NodeBaseG& node, Int stream) const
    {
        return NodeDispatcher::dispatch(node, GetSizeFn(), stream);
    }

    Int getBranchNodeSize(const NodeBaseG& node) const
    {
    	return BranchDispatcher::dispatch(node, GetSizeFn());
    }

    template <Int StreamIdx>
    Int getLeafStreamSize(const NodeBaseG& node) const
    {
    	return LeafDispatcher::dispatch(node, GetLeafNodeStreamSize<StreamIdx>());
    }

    Position getLeafStreamSizes(const NodeBaseG& node) const
    {
    	return LeafDispatcher::dispatch(node, GetLeafNodeStreamSizes());
    }


    Position getStreamSizes(const BranchNodeEntry& sums) const
    {
    	return LeafDispatcher::template dispatch<LeafNode>(true, GetLeafNodeStreamSizesStatic(), sums);
    }






MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::ToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS






M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::getNextNodeP(NodeBaseG& node) const
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParent(node);

        Int size = self.getNodeSize(parent, 0);

        Int parent_idx = node->parent_idx();

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

        Int parent_idx = node->parent_idx();

        if (parent_idx > 0)
        {
            return self.getChild(parent, parent_idx - 1);
        }
        else {
            NodeBaseG target_parent = getPrevNodeP(parent);

            if (target_parent.isSet())
            {
                Int node_size = self.getNodeSize(target_parent, 0);
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

}


#endif
