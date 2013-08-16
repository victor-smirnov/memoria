
// Copyright Victor Smirnov 2011-2013.
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

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position 											Position;

    typedef typename Types::TreePath                                            TreePath;
    typedef typename Types::TreePathItem                                        TreePathItem;

    static const Int Indexes                                                    = Types::Indexes;
    static const Int Streams                                                    = Types::Streams;

    void buildPath(TreePath& path, NodeBaseG node) const
    {
    	path.clear();

    	path.append(TreePathItem(node, node->parent_idx()));

    	while (!node->is_root())
    	{
    		node = getNodeParent(node, Allocator::READ);
    		path.append(TreePathItem(node, node->parent_idx()));
    	}
    }


    enum class BTNodeTraits {
        MAX_CHILDREN
    };

    template <typename Node>
    Int getNodeTraitsFn(BTNodeTraits trait, Int page_size) const
    {
    	switch (trait)
    	{
    		case BTNodeTraits::MAX_CHILDREN:
    			return Node::max_tree_size_for_block(page_size); break;

    		default: throw DispatchException(MEMORIA_SOURCE, "Unknown static node trait value", (Int)trait);
    	}
    }

    MEMORIA_CONST_STATIC_FN_WRAPPER_RTN(GetNodeTraitsFn, getNodeTraitsFn, Int);

    Int getNodeTraitInt(BTNodeTraits trait, bool leaf) const
    {
        Int page_size = self().getRootMetadata().page_size();
        return NonLeafDispatcher::template dispatchStaticRtn<BranchNode>(leaf, GetNodeTraitsFn(me()), trait, page_size);
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
        node.update();

        node->set_root(false);

        node->clearMetadata();
    }

    void node2Root(NodeBaseG& node, const Metadata& meta) const
    {
        node.update();

        node->set_root(true);

        node->parent_id().clear();
        node->parent_idx() = 0;

        node->setMetadata(meta);
    }

    void copyRootMetadata(NodeBaseG& src, NodeBaseG& tgt) const
    {
        tgt.update();
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
    NodeBaseG getChildFn(const Node* node, Int idx, Int flags) const
    {
    	return me()->allocator().getPage(node->value(idx), flags);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetChildFn, getChildFn, NodeBaseG);

    NodeBaseG getChild(const NodeBaseG& node, Int idx, Int flags) const
    {
    	NodeBaseG result = NonLeafDispatcher::dispatchConstRtn(node, GetChildFn(me()), idx, flags);

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


    TreePathItem& getParent(TreePath& path, const NodeBaseG& node) const
    {
        return path[node->level() + 1];
    }

    NodeBaseG getNodeParent(const NodeBaseG& node, Int flags = Allocator::READ) const
    {
    	return self().allocator().getPage(node->parent_id(), flags);
    }


    template <typename Node>
    Int getCapacityFn(const Node* node) const
    {
    	return node->capacity();
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetCapacityFn, getCapacityFn, Int);

    Int getCapacity(const NodeBaseG& node) const
    {
        return NonLeafDispatcher::dispatchConstRtn(node, GetCapacityFn(me()));
    }


    MEMORIA_DECLARE_NODE_FN_RTN(GetStreamCapacityFn, capacity, Int);

    Int getStreamCapacity(const NodeBaseG& node, Int stream) const
    {
    	Position reservation;
    	return getStreamCapacity(node, reservation, stream);
    }

    Int getStreamCapacity(const NodeBaseG& node, const Position& reservation, Int stream) const
    {
    	return LeafDispatcher::dispatchConstRtn(node, GetStreamCapacityFn(), reservation, stream);
    }


    MEMORIA_DECLARE_NODE_FN_RTN(GetNonLeafCapacityFn, capacity, Int);
    Int getNonLeafCapacity(const NodeBaseG& node, UBigInt active_streams) const
    {
    	return NonLeafDispatcher::dispatchConstRtn(node, GetNonLeafCapacityFn(), active_streams);
    }



    template <typename Node>
    Key getKeyFn(const Node* node, Int i, Int idx) const
    {
    	return node->map().key(i, idx);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetKeyFn, getKeyFn, Int);

    Key getKey(const NodeBaseG& node, Int i, Int idx) const
    {
        return NodeDispatcher::dispatchConstRtn(node, GetKeyFn(me()), i, idx);
    }

    Key getLeafKey(const NodeBaseG& node, Int i, Int idx) const
    {
    	return LeafDispatcher::dispatchConstRtn(node, GetKeyFn(me()), i, idx);
    }





    MEMORIA_DECLARE_NODE_FN_RTN(GetNonLeafKeysFn, keys, Accumulator);
    Accumulator getNonLeafKeys(const NodeBaseG& node, Int idx) const
    {
    	return NonLeafDispatcher::dispatchConstRtn(node, GetNonLeafKeysFn(), idx);
    }

    MEMORIA_DECLARE_NODE_FN_RTN(GetKeysFn, getKeys, Accumulator);
    Accumulator getLeafKeys(const NodeBaseG& node, Int idx) const
    {
    	return LeafDispatcher::dispatchConstRtn(node, GetKeysFn(), idx);
    }



    MEMORIA_DECLARE_NODE_FN_RTN(GetMaxKeyValuesFn, maxKeys, Accumulator);
    Accumulator getMaxKeys(const NodeBaseG& node) const
    {
    	return NonLeafDispatcher::dispatchConstRtn(node, GetMaxKeyValuesFn());
    }

    Accumulator getLeafMaxKeys(const NodeBaseG& node) const
    {
    	return LeafDispatcher::dispatchConstRtn(node, GetMaxKeyValuesFn());
    }




    MEMORIA_DECLARE_NODE_FN(SumsFn, sums);
    void sums(const NodeBaseG& node, Accumulator& sums) const
    {
    	NodeDispatcher::dispatchConst(node, SumsFn(), sums);
    }

    MEMORIA_DECLARE_NODE_FN_RTN(SumsRtnFn, sums, Accumulator);
    Accumulator sums(const NodeBaseG& node) const
    {
    	return NodeDispatcher::dispatchConstRtn(node, SumsRtnFn());
    }

    void sums(const NodeBaseG& node, Int start, Int end, Accumulator& sums) const
    {
    	NodeDispatcher::dispatchConst(node, SumsFn(), start, end, sums);
    }

    Accumulator sums(const NodeBaseG& node, Int start, Int end) const
    {
    	Accumulator sums;
    	NodeDispatcher::dispatchConst(node, SumsFn(), start, end, sums);
    	return sums;
    }

    void sums(const NodeBaseG& node, const Position& start, const Position& end, Accumulator& sums) const
    {
    	NodeDispatcher::dispatchConst(node, SumsFn(), start, end, sums);
    }

    Accumulator sums(const NodeBaseG& node, const Position& start, const Position& end) const
    {
    	Accumulator sums;
    	NodeDispatcher::dispatchConst(node, SumsFn(), start, end, sums);
    	return sums;
    }






    NodeBaseG getRoot(Int flags) const
    {
        return me()->allocator().getPage(me()->root(), flags);
    }


    MEMORIA_DECLARE_NODE_FN(SetKeysFn, setKeys);


    void setKeys(NodeBaseG& node, Int idx, const Accumulator& keys) const
    {
        node.update();
        NonLeafDispatcher::dispatch(node, SetKeysFn(), idx, keys);
    }

    void setNonLeafKeys(NodeBaseG& node, Int idx, const Accumulator& keys) const
    {
        node.update();
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
    	node.update();
    	NonLeafDispatcher::dispatch(node, SetChildrenCountFn(me()), count);
    }


    template <typename Node>
    ID getINodeDataFn(const Node* node, Int idx) const {
    	return node->value(idx);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetINodeDataFn, getINodeDataFn, ID);

    ID getChildID(const NodeBaseG& node, Int idx) const
    {
        return NonLeafDispatcher::dispatchConstRtn(node, GetINodeDataFn(me()), idx);
    }




    template <typename Node>
    void setChildID(Node* node, Int idx, const ID& id) const
    {
    	node->value(idx) = id;
    }

    MEMORIA_CONST_FN_WRAPPER(SetChildID, setChildID);

    void setChildID(NodeBaseG& node, Int idx, const ID& id) const
    {
        node.update();
        NonLeafDispatcher::dispatch(node, SetChildID(me()), idx, id);
    }


    template <typename Node>
    void reindexFn(Node* node) const
    {
    	node->reindex();
    }

    MEMORIA_CONST_FN_WRAPPER(ReindexFn, reindexFn);

    void reindex(NodeBaseG& node) const
    {
        node.update();
        NodeDispatcher::dispatch(node, ReindexFn(me()));
    }

    void reindexRegion(NodeBaseG& node, Int from, Int to) const
    {
        node.update();
        NodeDispatcher::dispatch(node, ReindexFn(me()));
    }








    void dump(const NodeBaseG& page, std::ostream& out = std::cout) const
    {
        if (page)
        {
            PageWrapper<const Page> pw(page);
            PageMetadata* meta = me()->getMetadata()->getPageMetadata(pw.getContainerHash(), pw.getPageTypeHash());
            memoria::vapi::dumpPage(meta, &pw, out);
            out<<endl;
            out<<endl;
        }
        else {
            out<<"NULL"<<std::endl;
        }
    }

    void dump(TreePath& path, Int level = 0, std::ostream& out = std::cout) const
    {
    	out<<"PATH of "<<path.getSize()<<" elements"<<endl;

    	for (Int c = path.getSize() - 1; c >=0; c--)
    	{
    		out<<"parentIdx = "<<path[c].parent_idx();
    		self().dump(path[c].node());
    	}

    }

    void dumpPath(NodeBaseG node, std::ostream& out = std::cout) const
    {
        auto& self = this->self();

    	out<<"Path:"<<endl;

        self.dump(node, out);

        while (!node->is_root())
        {
        	node = self.getNodeParent(node, Allocator::READ);
        	self.dump(node, out);
        }
    }

    Position getTotalKeyCount() const;
    void setTotalKeyCount(const Position& values);
    void addTotalKeyCount(const Position& values);
    void addTotalKeyCount(TreePath& path, const Position& value);

    bool getNextNode(TreePath& path, Int level = 0, bool down = false) const
    {
        Int idx = self().getNodeSize(path[level].node(), 0);
        return getNextNode(path, level, idx, down ? 0 : level );
    }

    bool getPrevNode(TreePath& path, Int level = 0, bool down = false) const
    {
        return getPrevNode(path, level, -1, down ? 0 : level);
    }

    NodeBaseG getNextNodeP(NodeBaseG& node) const;
    NodeBaseG getPrevNodeP(NodeBaseG& node) const;


    template <typename Node>
    Accumulator sumKeysFn(const Node* node, Int from, Int count) const
    {
    	Accumulator keys;
    	node->sum(from, from + count, keys);
    	return keys;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(SumKeysFn, sumKeysFn, Accumulator);




    template <typename Node>
    Key sumKeysInOneBlockFn(const Node* node, Int block_num, Int from, Int count) const
    {
    	Key keys = 0;
    	node->sum(block_num, from, from + count, keys);
    	return keys;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(SumKeysInOneBlockFn, sumKeysInOneBlockFn, Key);



    template <typename Node>
    void addKeysFn(Node* node, Int idx, const Accumulator& keys, bool reindex_fully) const
    {
    	node->updateUp(idx, keys);
    	node->reindex();
    }

    MEMORIA_CONST_FN_WRAPPER(AddKeysFn, addKeysFn);



    void sumKeys(const NodeBase *node, Int from, Int count, Accumulator& keys) const
    {
    	VectorAdd(keys, NonLeafDispatcher::dispatchConstRtn(node, SumKeysFn(me()), from, count));
    }

    void sumKeys(const NodeBase *node, Int block_num, Int from, Int count, Key& sum) const
    {
    	sum += NonLeafDispatcher::dispatchConstRtn(node, SumKeysInOneBlockFn(me()), block_num, from, count);
    }

    void addKeys(NodeBaseG& node, int idx, const Accumulator& keys, bool reindex_fully = false) const
    {
    	node.update();
    	NonLeafDispatcher::dispatch(node, AddKeysFn(me()), idx, keys, reindex_fully);
    }

    bool updateCounters(
    		TreePath& path,
    		Int level,
    		Int idx,
    		const Accumulator& counters,
    		std::function<void (Int, Int)> fn
    ) const;

    bool updateNodeCounters(NodeBaseG& node, Int idx, const Accumulator& counters) const;


    MEMORIA_DECLARE_NODE_FN_RTN(CheckCapacitiesFn, checkCapacities, bool);
    bool checkCapacities(const NodeBaseG& node, const Position& pos) const
    {
    	return NodeDispatcher::dispatchConstRtn(node, CheckCapacitiesFn(), pos);
    }


    MEMORIA_DECLARE_NODE_FN_RTN(GetSizesFn, sizes, Position);
    Position getNodeSizes(const NodeBaseG& node) const
    {
    	return NodeDispatcher::dispatchConstRtn(node, GetSizesFn());
    }

    MEMORIA_DECLARE_NODE_FN_RTN(GetSizeFn, size, Int);
    Int getNodeSize(const NodeBaseG& node, Int stream) const
    {
    	return NodeDispatcher::dispatchConstRtn(node, GetSizeFn(), stream);
    }

    MEMORIA_DECLARE_NODE_FN(LayoutNodeFn, layout);
    void layoutNonLeafNode(NodeBaseG& node, UBigInt active_streams) const
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
    	return NodeDispatcher::dispatchConstRtn(node, GetActiveStreamsFn());
    }

    void initLeaf(NodeBaseG& node) const {}

    MEMORIA_DECLARE_NODE_FN_RTN(IsNodeEmpty, is_empty, bool);
    bool isNodeEmpty(const NodeBaseG& node)
    {
    	return NodeDispatcher::dispatchConstRtn(node, IsNodeEmpty());
    }


private:

    bool getNextNode(TreePath& path, Int level, Int idx, Int target_level) const;
    bool getPrevNode(TreePath& path, Int level, Int idx, Int target_level) const;



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
void M_TYPE::addTotalKeyCount(TreePath& path, const Position& values)
{
    auto& self 			= this->self();

    NodeBaseG& node     = path[path.getSize() - 1].node();

    Metadata meta       = self.getRootMetadata();

    for (Int c = 0; c < Streams; c++)
    {
    	meta.size(c) += values[c];
    }

    self.setRootMetadata(node, meta);
}





M_PARAMS
bool M_TYPE::getNextNode(TreePath& path, Int level, Int idx, Int target_level) const
{
    auto& self = this->self();

	NodeBaseG& page = path[level].node();

    if (idx < self.getNodeSize(page, 0))
    {
        for(; level != target_level && level > 0; level--)
        {
            path[level - 1].node()          = self.getChild(path[level].node(), idx, Allocator::READ);
            path[level - 1].parent_idx()    = idx;

            idx = 0;
        }

        return true;
    }
    else {
        if (!page->is_root())
        {
            return getNextNode(path, level + 1, path[level].parent_idx() + 1, target_level);
        }
    }

    return false;
}


M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::getNextNodeP(NodeBaseG& node) const
{
    auto& self = this->self();

    if (!node->is_root())
    {
        NodeBaseG parent = self.getNodeParent(node, Allocator::READ);

    	Int size = self.getNodeSize(parent, 0);

    	Int parent_idx = node->parent_idx();

        if (parent_idx < size - 1)
        {
        	return self.getChild(parent, parent_idx + 1, Allocator::READ);
        }
        else {
        	NodeBaseG target_parent = getNextNodeP(parent);

        	if (target_parent.isSet())
        	{
        		return self.getChild(target_parent, 0, Allocator::READ);
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
        NodeBaseG parent = self.getNodeParent(node, Allocator::READ);

    	Int parent_idx = node->parent_idx();

        if (parent_idx > 0)
        {
        	return self.getChild(parent, parent_idx - 1, Allocator::READ);
        }
        else {
        	NodeBaseG target_parent = getPrevNodeP(parent);

        	if (target_parent.isSet())
        	{
        		Int node_size = self.getNodeSize(target_parent, 0);
        		return self.getChild(target_parent, node_size - 1, Allocator::READ);
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
bool M_TYPE::getPrevNode(TreePath& path, Int level, Int idx, Int target_level) const
{
	auto& self = this->self();

	NodeBaseG& page = path[level].node();

    if (idx >= 0)
    {
        for(; level != target_level && level > 0; level--)
        {
            path[level - 1].node()          = self.getChild(path[level].node(), idx, Allocator::READ);
            path[level - 1].parent_idx()    = idx;

            idx = self.getNodeSize(path[level - 1].node(), 0) - 1;
        }

        return true;
    }
    else {
        if (!page->is_root())
        {
            return getPrevNode(path, level + 1, path[level].parent_idx() - 1, target_level);
        }
    }

    return false;
}



M_PARAMS
bool M_TYPE::updateCounters(
		TreePath& path,
		Int level,
		Int idx,
		const Accumulator& counters,
		std::function<void (Int, Int)> fn
) const
{
    auto& self = this->self();

	path[level].node().update();
    self.addKeys(path[level], idx, counters, true);

    return false; //proceed further unconditionally
}

M_PARAMS
bool M_TYPE::updateNodeCounters(NodeBaseG& node, Int idx, const Accumulator& counters) const
{
    node.update();
    self().addKeys(node, idx, counters, true);
    return false;
}



#undef M_TYPE
#undef M_PARAMS

}


#endif
