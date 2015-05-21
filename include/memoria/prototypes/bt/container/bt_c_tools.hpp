
// Copyright Victor Smirnov 2011-2014.
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

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;
    typedef typename Base::NonRootDispatcher                                    NonRootDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::Source                                              Source;
    typedef typename Types::Target                                              Target;
    typedef typename Types::Entropy                                             Entropy;

    static const Int Streams                                                    = Types::Streams;

    enum class BTNodeTraits {
        MAX_CHILDREN
    };

    template <typename Node>
    Int getNodeTraitsFn(BTNodeTraits trait, Int page_size) const
    {
        switch (trait)
        {
            case BTNodeTraits::MAX_CHILDREN:
                return Node::max_tree_size_for_block(page_size, false); break;

            default: throw DispatchException(MEMORIA_SOURCE, "Unknown static node trait value", (Int)trait);
        }
    }

    MEMORIA_CONST_STATIC_FN_WRAPPER_RTN(GetNodeTraitsFn, getNodeTraitsFn, Int);

    Int getNodeTraitInt(BTNodeTraits trait, bool leaf) const
    {
        Int page_size = self().getRootMetadata().page_size();
        return NonLeafDispatcher::template dispatch<BranchNode>(leaf, GetNodeTraitsFn(me()), trait, page_size);
    }



    Int getMaxKeyCountForNode(bool leaf, Int level) const
    {
        return getNodeTraitInt(BTNodeTraits::MAX_CHILDREN, leaf);
    }


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
    ID getPageIdFn(const Node* node, Int idx) const
    {
        return node->map().data(idx);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetPageIdFn, getPageIdFn, ID);

    ID getPageId(const NodeBaseG& node, Int idx) const
    {
        return NonLeafDispatcher::dispatchConstRtn(node, GetPageIdFn(me()), idx);
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
        NodeBaseG result = NonLeafDispatcher::dispatch(node, GetChildFn(me()), idx);

        if (!result.isEmpty())
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
        NodeBaseG result = NonLeafDispatcher::dispatch(node, GetChildForUpdateFn(me()), idx);

        if (!result.isEmpty())
        {
            return result;
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Child must not be NULL");
        }
    }


    NodeBaseG getLastChild(const NodeBaseG& node, Int flags) const
    {
        return getChild(node, self().getNodeSize(node, 0) - 1, flags);
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


    template <typename Node>
    Int getCapacityFn(const Node* node) const
    {
        return node->capacity();
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetCapacityFn, getCapacityFn, Int);

    Int getCapacity(const NodeBaseG& node) const
    {
        return NonLeafDispatcher::dispatch(node, GetCapacityFn(me()));
    }


    MEMORIA_DECLARE_NODE_FN_RTN(GetStreamCapacityFn, capacity, Int);

    Int getStreamCapacity(const NodeBaseG& node, Int stream) const
    {
        Position reservation;
        return getStreamCapacity(node, reservation, stream);
    }

    Int getStreamCapacity(const NodeBaseG& node, const Position& reservation, Int stream) const
    {
        return LeafDispatcher::dispatch(node, GetStreamCapacityFn(), reservation, stream);
    }


    MEMORIA_DECLARE_NODE_FN_RTN(GetNonLeafCapacityFn, capacity, Int);
    Int getNonLeafCapacity(const NodeBaseG& node, UBigInt active_streams) const
    {
        return NonLeafDispatcher::dispatch(node, GetNonLeafCapacityFn(), active_streams);
    }




    MEMORIA_DECLARE_NODE_FN(SumsFn, sums);
    void sums(const NodeBaseG& node, Accumulator& sums) const
    {
        NodeDispatcher::dispatch(node, SumsFn(), sums);
    }

    MEMORIA_DECLARE_NODE_FN_RTN(SumsRtnFn, sums, Accumulator);
    Accumulator sums(const NodeBaseG& node) const
    {
        return NodeDispatcher::dispatch(node, SumsRtnFn());
    }

    void sums(const NodeBaseG& node, Int start, Int end, Accumulator& sums) const
    {
        NodeDispatcher::dispatch(node, SumsFn(), start, end, sums);
    }

    Accumulator sums(const NodeBaseG& node, Int start, Int end) const
    {
        Accumulator sums;
        NodeDispatcher::dispatch(node, SumsFn(), start, end, sums);
        return sums;
    }

    void sums(const NodeBaseG& node, const Position& start, const Position& end, Accumulator& sums) const
    {
        NodeDispatcher::dispatch(node, SumsFn(), start, end, sums);
    }

    Accumulator sums(const NodeBaseG& node, const Position& start, const Position& end) const
    {
        Accumulator sums;
        NodeDispatcher::dispatch(node, SumsFn(), start, end, sums);
        return sums;
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


//    void setKeys(NodeBaseG& node, Int idx, const Accumulator& keys) const
//    {
//        self().updatePageG(node);
//        NonLeafDispatcher::dispatch(node, SetKeysFn(), idx, keys);
//    }

    void setNonLeafKeys(NodeBaseG& node, Int idx, const Accumulator& keys) const
    {
        self().updatePageG(node);
        NonLeafDispatcher::dispatch(node, SetKeysFn(), idx, keys);
    }




    template <typename Node>
    void setChildrenCountFn(Node* node, Int count) const
    {
        node->set_children_count1(count);
    }

    MEMORIA_CONST_FN_WRAPPER(SetChildrenCountFn, setChildrenCountFn);

    void setNonLeafChildrenCount(NodeBaseG& node, Int count) const
    {
        self().updatePageG(node);
        NonLeafDispatcher::dispatch(node, SetChildrenCountFn(me()), count);
    }


    template <typename Node>
    ID getINodeDataFn(const Node* node, Int idx) const {
        return node->value(idx);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetINodeDataFn, getINodeDataFn, ID);

    ID getChildID(const NodeBaseG& node, Int idx) const
    {
        return NonLeafDispatcher::dispatch(node, GetINodeDataFn(me()), idx);
    }


    template <typename Node>
    void setChildID(Node* node, Int idx, const ID& id) const
    {
        node->value(idx) = id;
    }

    MEMORIA_CONST_FN_WRAPPER(SetChildID, setChildID);

    void setChildID(NodeBaseG& node, Int idx, const ID& id) const
    {
        self().updatePageG(node);
        NonLeafDispatcher::dispatch(node, SetChildID(me()), idx, id);
    }

    MEMORIA_DECLARE_NODE_FN(ReindexFn, reindex);

    void reindex(NodeBaseG& node) const
    {
        self().updatePageG(node);
        NodeDispatcher::dispatch(node, ReindexFn());
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

    void dumpPath(NodeBaseG node, std::ostream& out = std::cout) const
    {
        auto& self = this->self();

        out<<"Path:"<<std::endl;

        self.dump(node, out);

        while (!node->is_root())
        {
            node = self.getNodeParent(node);
            self.dump(node, out);
        }
    }

    Position getTotalKeyCount() const;
    void setTotalKeyCount(const Position& values);
    void addTotalKeyCount(const Position& values);


    NodeBaseG getNextNodeP(NodeBaseG& node) const;
    NodeBaseG getPrevNodeP(NodeBaseG& node) const;

    MEMORIA_DECLARE_NODE_FN(AddKeysFn, updateUp);
    void addKeys(NodeBaseG& node, int idx, const Accumulator& keys)
    {
        self().updatePageG(node);
        NonLeafDispatcher::dispatch(node, AddKeysFn(me()), idx, keys);
    }


    MEMORIA_DECLARE_NODE_FN_RTN(CheckCapacitiesFn, checkCapacities, bool);
    bool checkCapacities(const NodeBaseG& node, const Position& sizes) const
    {
        return NodeDispatcher::dispatch(node, CheckCapacitiesFn(), sizes);
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
    	return NonLeafDispatcher::dispatch(node, GetSizeFn());
    }

    MEMORIA_DECLARE_NODE_FN(LayoutNodeFn, layout);
    void layoutBranchNode(NodeBaseG& node, UBigInt active_streams) const
    {
        NonLeafDispatcher::dispatch(node, LayoutNodeFn(), active_streams);
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
        NonLeafDispatcher::dispatch(node, ForAllIDsFn(), start, end, fn);
    }

    void forAllIDs(const NodeBaseG& node, Int start, std::function<void (const ID&, Int)> fn) const
    {
    	NonLeafDispatcher::dispatch(node, ForAllIDsFn(), start, fn);
    }

    void forAllIDs(const NodeBaseG& node, std::function<void (const ID&, Int)> fn) const
    {
    	NonLeafDispatcher::dispatch(node, ForAllIDsFn(), fn);
    }

    struct GetRemainderSize {
        template <Int Idx, typename Stream>
        void operator()(const Stream* obj, Position& pos)
        {
            pos[Idx] = obj->getRemainder();
        }
    };

    struct GetStreamPositionFn {
        template <Int Idx, typename Stream>
        void operator()(const Stream* obj, Position& pos)
        {
            pos[Idx] = obj->getStart();
        }
    };

    struct ResetPositionFn {
        template <Int Idx, typename Stream>
        void operator()(Stream* obj, const Position& pos)
        {
            obj->reset(pos[Idx]);
        }
    };

    Position getRemainderSize(const Source& source)
    {
        Position pos;

        TupleDispatcher<Source>::dispatch(source, GetRemainderSize(), pos);

        return pos;
    }

    Position getStreamPosition(const Source& source)
    {
        Position pos;

        TupleDispatcher<Source>::dispatch(source, GetStreamPositionFn(), pos);

        return pos;
    }

    void setStreamPosition(Source& source, const Position& pos)
    {
        TupleDispatcher<Source>::dispatch(source, ResetPositionFn(), pos);
    }

    Position getRemainderSize(const Target& target)
    {
        Position pos;

        TupleDispatcher<Target>::dispatch(target, GetRemainderSize(), pos);

        return pos;
    }

    MEMORIA_DECLARE_NODE_FN_RTN(EstimateEntropy, estimateEntropy, bool);
    Entropy estimateLeafEntropy(const NodeBaseG& node, const Position& start, const Position& end)
    {
        Entropy entropy;

        LeafDispatcher::dispatch(node, EstimateEntropy(), entropy, start, end);

        return entropy;
    }

    Entropy estimateLeafEntropy(const NodeBaseG& node)
    {
        Entropy entropy;

        LeafDispatcher::dispatch(node, EstimateEntropy(), entropy);

        return entropy;
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

    Int getNodeChildrenCount(const NodeBaseG& node) const
    {
    	return NonLeafDispatcher::dispatch(node, GetBranchNodeChildernCount());
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


    template <Int Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto _applySubstreamsFn(NodeBaseG& leaf, Fn&& fn, Args&&... args)
    -> DispatchRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, Fn, Args...>
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <Int Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto _applySubstreamsFn(const NodeBaseG& leaf, Fn&& fn, Args&&... args) const
    -> DispatchConstRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, Fn, Args...>
    {
    	return LeafDispatcher::dispatch(leaf, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::ToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
typename M_TYPE::Position M_TYPE::getTotalKeyCount() const
{
    auto& self = this->self();

    Position sizes;

    Metadata meta = self.getRootMetadata();

    for (Int c = 0; c < Streams; c++)
    {
        sizes[c] = meta.size(c);
    }

    return sizes;
}

M_PARAMS
void M_TYPE::setTotalKeyCount(const Position& values)
{
    auto& self = this->self();

    Metadata meta = self.getRootMetadata();

    for (Int c = 0; c < Streams; c++)
    {
        meta.size(c) = values[c];
    }

    self.setRootMetadata(meta);
}

M_PARAMS
void M_TYPE::addTotalKeyCount(const Position& values)
{
    auto& self = this->self();

    Metadata meta = self.getRootMetadata();

    for (Int c = 0; c < Streams; c++)
    {
        meta.size(c) += values[c];
    }

    self.setRootMetadata(meta);
}



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
