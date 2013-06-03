
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_TOOLS_HPP

#include <memoria/metadata/tools.hpp>
#include <memoria/core/tools/strings.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/balanced_tree/nodes/tree_node.hpp>
#include <memoria/prototypes/balanced_tree/baltree_macros.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::balanced_tree;



MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::ToolsName)



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

    enum class BalTreeNodeTraits {
        MAX_CHILDREN
    };

    template <typename Node>
    Int getNodeTraitsFn(BalTreeNodeTraits trait, Int page_size) const
    {
    	switch (trait)
    	{
    		case BalTreeNodeTraits::MAX_CHILDREN:
    			return Node::max_tree_size_for_block(page_size); break;

    		default: throw DispatchException(MEMORIA_SOURCE, "Unknown static node trait value", (Int)trait);
    	}
    }

    MEMORIA_CONST_STATIC_FN_WRAPPER_RTN(GetNodeTraitsFn, getNodeTraitsFn, Int);

    Int getNodeTraitInt(BalTreeNodeTraits trait, bool root, bool leaf) const
    {
        Int page_size = me()->getRootMetadata().page_size();
        return NonLeafDispatcher::template dispatchStaticRtn<TreeMapNode>(root, leaf, GetNodeTraitsFn(me()), trait, page_size);
    }



    Int getMaxKeyCountForNode(bool root, bool leaf, Int level) const
    {
        Int key_count = getNodeTraitInt(BalTreeNodeTraits::MAX_CHILDREN, root, leaf);
        Int max_count = me()->getBranchingFactor();

        if (max_count == 0)
        {
            return key_count;
        }
        else {
            return key_count < max_count? key_count : max_count;
        }
    }


    bool isTheSameNode(const TreePath& path1, const TreePath& path2, int level) const
    {
        return path1[level].node()->id() == path2[level].node()->id();
    }

    void setBranchingFactor(Int count)
    {
        if (count == 0 || count > 2)
        {
            Metadata meta           = me()->getRootMetadata();
            meta.branching_factor() = count;

            me()->setRootMetadata(meta);
        }
        else {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Incorrect setBranchingFactor value: "<<count<<". It must be 0 or > 2");
        }
    }

    Int getBranchingFactor() const
    {
        return me()->getRootMetadata().branching_factor();
    }



    template <typename Node>
    void root2NodeFn(Node* src) const
    {
    	typedef typename Node::NonRootNodeType NonRootNode;

    	NonRootNode* tgt = T2T<NonRootNode*>(malloc(src->page_size()));
    	memset(tgt, 0, src->page_size());

    	ConvertRootToNode(src, tgt);

    	CopyByteBuffer(tgt, src, tgt->page_size());

    	free(tgt);
    }

    MEMORIA_CONST_FN_WRAPPER(Root2NodeFn0, template root2NodeFn);


    void root2Node(NodeBaseG& node) const
    {
        node.update();
        RootDispatcher::dispatch(node, Root2NodeFn0(me()));
    }


    template <typename Node>
    void node2RootFn(Node* src, const Metadata& metadata) const
    {
    	typedef typename Node::RootNodeType RootType;

    	RootType* tgt = T2T<RootType*>(malloc(src->page_size()));
    	memset(tgt, 0, src->page_size());

    	ConvertNodeToRoot(src, tgt);

    	tgt->root_metadata() = metadata;

    	CopyByteBuffer(tgt, src, tgt->page_size());

    	free(tgt);
    }

    MEMORIA_CONST_FN_WRAPPER(Node2RootFn, node2RootFn);

    void node2Root(NodeBaseG& node, const Metadata& meta) const
    {
        node.update();
        NonRootDispatcher::dispatch(node, Node2RootFn(me()), meta);
    }



    template <typename Node1, typename Node2>
    void copyRootMetadataFn(Node1* src, Node2* tgt) const
    {
    	tgt->root_metadata() = src->root_metadata();
    }

    MEMORIA_CONST_FN_WRAPPER(CopyRootMetadataFn, copyRootMetadataFn);

    void copyRootMetadata(NodeBaseG& src, NodeBaseG& tgt) const
    {
        tgt.update();
        RootDispatcher::doubleDispatch(src, tgt, CopyRootMetadataFn(me()));
    }




    template <typename T>
    bool canConvertToRootFn(const T* node) const
    {
    	typedef typename T::RootNodeType RootType;

    	Int node_children_count = node->size(0);

    	Int root_block_size 	= node->page_size();

    	Int root_children_count = RootType::max_tree_size_for_block(root_block_size);

    	return node_children_count <= root_children_count;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(CanConvertToRootFn, canConvertToRootFn, bool);

    bool canConvertToRoot(const NodeBaseG& node) const
    {
        return NonRootDispatcher::dispatchConstRtn(node, CanConvertToRootFn(me()));
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



    MEMORIA_DECLARE_NODE_FN_RTN(GetKeysFn, getKeys, Accumulator);

    Accumulator getKeys(const NodeBaseG& node, Int idx) const
    {
    	return NodeDispatcher::dispatchConstRtn(node, GetKeysFn(), idx);
    }

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


    NodeBaseG getRoot(Int flags) const
    {
        return me()->allocator().getPage(me()->root(), flags);
    }


    MEMORIA_DECLARE_NODE_FN(SetKeysFn, setKeys);


    void setKeys(NodeBaseG& node, Int idx, const Accumulator& keys) const
    {
        node.update();
        NodeDispatcher::dispatch(node, SetKeysFn(), idx, keys);
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
        if (page != NULL)
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

    void dump(TreePath& path, Int level = 0, std::ostream& out = std::cout)
    {
    	out<<"PATH of "<<path.getSize()<<" elements"<<endl;

    	for (Int c = path.getSize() - 1; c >=0; c--)
    	{
    		out<<"parentIdx = "<<path[c].parent_idx();
    		self().dump(path[c].node());
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


    template <typename Node>
    bool checkNodeContent(Node *node);


    template <typename Node1, typename Node2>
    bool checkNodeWithParentContent(Node1 *node, Node2 *parent, Int parent_idx);



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

    bool updateCounters(TreePath& path, Int level, Int idx, const Accumulator& counters, bool reindex_fully = false) const;
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

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::ToolsName)
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
bool M_TYPE::updateCounters(TreePath& path, Int level, Int idx, const Accumulator& counters, bool reindex_fully) const
{
    auto& self = this->self();

	path[level].node().update();
    self.addKeys(path[level], idx, counters, reindex_fully);

    return false; //proceed further unconditionally
}

M_PARAMS
bool M_TYPE::updateNodeCounters(NodeBaseG& node, Int idx, const Accumulator& counters) const
{
    node.update();
    self().addKeys(node, idx, counters, true);
    return false;
}


M_PARAMS
template <typename Node>
bool M_TYPE::checkNodeContent(Node *node) {
    bool errors = false;

    for (Int i = 0; i < Indexes; i++) {
        Key key = 0;

        for (Int c = 0; c < self().getNodeSize(node, 0); c++) {
            key += node->map().key(i, c);
        }

        if (key != node->map().maxKey(i))
        {
            //me()->dump(node);
            MEMORIA_ERROR(me(), "Sum of keys doen't match maxKey for key", i, key, node->map().maxKey(i));
            errors = true;
        }
    }

    return errors;
}

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::checkNodeWithParentContent(Node1 *node, Node2 *parent, Int parent_idx)
{
    bool errors = false;
    for (Int c = 0; c < Indexes; c++)
    {
        if (node->map().maxKey(c) != parent->map().key(c, parent_idx))
        {
            MEMORIA_ERROR(
                    me(),
                    "Invalid parent-child nodes chain",
                    c,
                    node->map().maxKey(c),
                    parent->map().key(c, parent_idx),
                    "for",
                    node->id(),
                    parent->id(),
                    parent_idx
            );

            errors = true;
        }
    }

    errors = checkNodeContent(node) || errors;

    return errors;
}



#undef M_TYPE
#undef M_PARAMS

}


#endif
