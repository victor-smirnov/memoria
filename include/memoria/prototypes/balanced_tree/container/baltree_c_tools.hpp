
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_TOOLS_HPP

#include <memoria/prototypes/balanced_tree/pages/pages.hpp>

#include <memoria/metadata/tools.hpp>
#include <memoria/core/tools/strings.hpp>
#include <memoria/core/container/macros.hpp>

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

    typedef typename Base::NodeBase::Base                                       TreeNodePage;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;
    typedef typename Base::NonRootDispatcher                                    NonRootDispatcher;

    typedef typename Base::Node2RootMap                                         Node2RootMap;
    typedef typename Base::Root2NodeMap                                         Root2NodeMap;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;
    typedef typename Base::Accumulator                                          Accumulator;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int Indexes                                                    = Types::Indexes;


    enum class BTreeNodeTraits {
        MAX_CHILDREN
    };



    template <typename Node>
    Int getNodeTraitsFn(BTreeNodeTraits trait, Int page_size) const
    {
    	switch (trait)
    	{
    		case BTreeNodeTraits::MAX_CHILDREN:
    			return Node::Map::max_tree_size(page_size - sizeof(Node) + sizeof(typename Node::Map)); break;

    		default: throw DispatchException(MEMORIA_SOURCE, "Unknown static node trait value", trait);
    	}
    }

    MEMORIA_CONST_STATIC_FN_WRAPPER_RTN(GetNodeTraitsFn, getNodeTraitsFn, Int);

    Int getNodeTraitInt(BTreeNodeTraits trait, bool root, bool leaf, Int level) const
    {
        Int page_size = me()->getRootMetadata().page_size();
        return NodeDispatcher::dispatchStaticRtn(root, leaf, level, GetNodeTraitsFn(me()), trait, page_size);
    }



    Int getMaxKeyCountForNode(bool root, bool leaf, Int level) const
    {
        Int key_count = getNodeTraitInt(BTreeNodeTraits::MAX_CHILDREN, root, leaf, level);
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
    	typedef typename memoria::Type2TypeMap<Node, Root2NodeMap>::Result NonRootNode;

    	NonRootNode* tgt = T2T<NonRootNode*>(malloc(src->page_size()));
    	memset(tgt, 0, src->page_size());

    	tgt->map().init(src->page_size() - sizeof(NonRootNode) + sizeof(typename NonRootNode::Map));
    	tgt->copyFrom(src);
    	tgt->page_type_hash()   = NonRootNode::hash();
    	tgt->set_root(false);

    	src->map().transferDataTo(&tgt->map());

    	tgt->set_children_count(src->children_count());

    	tgt->map().clearUnused();

    	tgt->map().reindex();

    	CopyByteBuffer(tgt, src, tgt->page_size());

    	free(tgt);
    }

    MEMORIA_CONST_FN_WRAPPER(Root2NodeFn0, root2NodeFn);


    void root2Node(NodeBaseG& node) const
    {
        node.update();
        RootDispatcher::dispatch(node, Root2NodeFn0(me()));
    }


    template <typename Node>
    void node2RootFn(Node* src, const Metadata& metadata) const
    {
    	typedef typename memoria::Type2TypeMap<Node, Node2RootMap>::Result RootType;

    	RootType* tgt = T2T<RootType*>(malloc(src->page_size()));
    	memset(tgt, 0, src->page_size());

    	tgt->map().init(src->page_size() - sizeof(RootType) + sizeof(typename RootType::Map));
    	tgt->copyFrom(src);

    	tgt->root_metadata() = metadata;

    	tgt->set_root(true);

    	tgt->page_type_hash()   = RootType::hash();

    	src->map().transferDataTo(&tgt->map());

    	tgt->set_children_count(src->children_count());

    	tgt->map().clearUnused();

    	tgt->map().reindex();

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
    	typedef typename memoria::Type2TypeMap<T, Node2RootMap, void>::Result RootType;

    	Int node_children_count = node->children_count();

    	Int root_block_size 	= node->page_size() - sizeof(RootType) + sizeof(typename RootType::Map);

    	Int root_children_count = RootType::Map::max_tree_size(root_block_size);

    	return node_children_count <= root_children_count;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(CanConvertToRootFn, canConvertToRootFn, bool);

    bool canConvertToRoot(NodeBase* node) const
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
    	return me()->allocator().getPage(node->map().data(idx), flags);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetChildFn, getChildFn, NodeBaseG);

    NodeBaseG getChild(const NodeBase *node, Int idx, Int flags) const
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

    NodeBaseG getLastChild(const NodeBase *node, Int flags) const
    {
        return getChild(node, node->children_count() - 1, flags);
    }


    TreePathItem& getParent(TreePath& path, const NodeBaseG& node) const
    {
        return path[node->level() + 1];
    }


    template <typename Node>
    Int getCapacityFn(const Node* node, Int max_node_capacity) const
    {
    	if (max_node_capacity == 0)
    	{
    		return (node->map().maxSize() - node->children_count());
    	}
    	else
    	{
    		Int capacity = max_node_capacity - node->children_count();
    		return capacity > 0 ? capacity : 0;
    	}
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetCapacityFn, getCapacityFn, Int);

    Int getCapacity(const NodeBaseG& node) const
    {
        return NodeDispatcher::dispatchConstRtn(node, GetCapacityFn(me()), me()->getBranchingFactor());
    }

    template <typename Node>
    Int getMapCapacityFn(const Node* node, Int max_node_capacity) const
    {
    	if (max_node_capacity == 0)
    	{
    		return node->map().maxSize();
    	}
    	else
    	{
    		return max_node_capacity;
    	}
    }


    MEMORIA_CONST_FN_WRAPPER_RTN(GetMaxCapacityFn, getMaxCapacityFn, Int);

    Int getMaxCapacity(const NodeBaseG& node) const
    {
    	return NodeDispatcher::dispatchConstRtn(node, GetCapacityFn(me()), me()->getBranchingFactor());
    }

    bool shouldMergeNode(const TreePath& path, Int level) const
    {
        const NodeBaseG& node = path[level].node();
        return node->children_count() <= me()->getMaxCapacity(node) / 2;
    }

    bool shouldSplitNode(const TreePath& path, Int level) const
    {
        const NodeBaseG& node = path[level].node();
        return me()->getCapacity(node) == 0;
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




    template <typename Node>
    void extractKeyValuesFn(const Node* node, Int idx, Key* keys) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		keys[c] = node->map().key(c, idx);
    	}
    }

    MEMORIA_CONST_FN_WRAPPER(ExtractKeyValuesFn, extractKeyValuesFn);


    Accumulator getKeys(const NodeBaseG& node, Int idx) const
    {
        Accumulator keys;
        NodeDispatcher::dispatchConst(node, ExtractKeyValuesFn(me()), idx, keys.keys());

        return keys;
    }





    template <typename Node>
    void extractMaxKeyValuesFn(const Node* node, Key* keys) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		keys[c] = node->map().maxKey(c);
    	}
    }

    MEMORIA_CONST_FN_WRAPPER(ExtractMaxKeyValuesFn, extractMaxKeyValuesFn);

    Accumulator getMaxKeys(const NodeBaseG& node) const
    {
    	Accumulator keys;
    	NodeDispatcher::dispatchConst(node, ExtractMaxKeyValuesFn(me()), keys.keys());

    	return keys;
    }




    NodeBaseG getRoot(Int flags) const
    {
        return me()->allocator().getPage(me()->root(), flags);
    }



    template <typename Node>
    void setKeysFn(Node* node, Int idx, const Accumulator& keys) const
    {
    	for (Int c = 0; c < Node::Map::Blocks; c++)
    	{
    		node->map().key(c, idx) = keys[c];
    	}
    }

    MEMORIA_CONST_FN_WRAPPER(SetKeysFn, setKeysFn);


    void setKeys(NodeBaseG& node, Int idx, const Accumulator& keys) const
    {
        node.update();
        NodeDispatcher::dispatch(node, SetKeysFn(me()), idx, keys);
    }




    template <typename Node>
    void setChildrenCountFn(Node* node, Int count) const
    {
    	node->set_children_count(count);
    }

    MEMORIA_CONST_FN_WRAPPER(SetChildrenCountFn, setChildrenCountFn);

    void setChildrenCount(NodeBaseG& node, Int count) const
    {
        node.update();
        NodeDispatcher::dispatch(node, SetChildrenCountFn(me()), count);
    }


    template <typename Node>
    void addChildrenCountFn(Node* node, Int count) const
    {
    	node->inc_size(count);
    }

    MEMORIA_CONST_FN_WRAPPER(AddChildrenCountFn, addChildrenCountFn);

    void addChildrenCount(NodeBaseG& node, Int count) const
    {
        node.update();
        NodeDispatcher::dispatch(node, AddChildrenCountFn(me()), count);
    }



    template <typename Node>
    ID getINodeDataFn(const Node* node, Int idx) const {
    	return node->map().data(idx);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetINodeDataFn, getINodeDataFn, ID);

    ID getChildID(const NodeBaseG& node, Int idx) const
    {
        return NonLeafDispatcher::dispatchConstRtn(node, GetINodeDataFn(me()), idx);
    }




    template <typename Node>
    void setChildID(Node* node, Int idx, const ID& id) const
    {
    	node->map().data(idx) = id;
    }

    MEMORIA_CONST_FN_WRAPPER(SetChildID, setChildID);

    void setChildID(NodeBaseG& node, Int idx, const ID& id) const
    {
        node.update();
        NonLeafDispatcher::dispatch(node, SetChildID(me()), idx, id);
    }


    template <typename Node>
    void reindexFn(Node* node, Int from, Int to) const
    {
    	node->map().reindexAll(from, to);
    }

    MEMORIA_CONST_FN_WRAPPER(ReindexFn, reindexFn);

    void reindex(NodeBaseG& node) const
    {
        node.update();
        NodeDispatcher::dispatch(node, ReindexFn(me()), 0, node->children_count());
    }

    void reindexRegion(NodeBaseG& node, Int from, Int to) const
    {
        node.update();
        NodeDispatcher::dispatch(node, ReindexFn(me()), from, to);
    }




    template <typename Node>
    void setAndReindexFn(Node* node, Int idx, const Element& element) const
    {
    	for (Int c = 0; c < MyType::Indexes; c++)
    	{
    		node->map().key(c, idx) = element.first[c];
    	}

    	node->map().data(idx) = element.second;

    	if (idx == node->children_count() - 1)
    	{
    		node->map().reindexAll(idx, idx + 1);
    	}
    	else {
    		node->map().reindexAll(idx, node->children_count());
    	}
    }

    MEMORIA_CONST_FN_WRAPPER(SetAndReindexFn, setAndReindexFn);


    void setLeafDataAndReindex(NodeBaseG& node, Int idx, const Element& element) const
    {
        node.update();
        LeafDispatcher::dispatch(node.page(), SetAndReindexFn(me()), idx, element);
    }
    

    template <typename Node>
    Value getLeafDataFn(const Node* node, Int idx) const
    {
    	return node->map().data(idx);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetLeafDataFn, getLeafDataFn, Value);

    Value getLeafData(const NodeBaseG& node, Int idx) const
    {
        return LeafDispatcher::dispatchConstRtn(node.page(), GetLeafDataFn(me()), idx);
    }




    template <typename Node>
    void setLeafDataFn(Node* node, Int idx, const Value& val) const
    {
    	node->map().data(idx) = val;
    }

    MEMORIA_CONST_FN_WRAPPER(SetLeafDataFn, setLeafDataFn);



    void setLeafData(NodeBaseG& node, Int idx, const Value &val) const
    {
        node.update();
        LeafDispatcher::dispatch(node.page(), SetLeafDataFn(me()), idx, val);
    }





    void dump(PageG page, std::ostream& out = std::cout) const
    {
        if (page != NULL)
        {
            PageWrapper<Page> pw(page);
            PageMetadata* meta = me()->getMetadata()->getPageMetadata(pw.getContainerHash(), pw.getPageTypeHash());
            memoria::vapi::dumpPage(meta, &pw, out);
            out<<endl;
            out<<endl;
        }
        else {
            out<<"NULL"<<std::endl;
        }
    }

    BigInt getTotalKeyCount() const;
    void setTotalKeyCount(BigInt value);
    void addTotalKeyCount(BigInt value);
    void addTotalKeyCount(TreePath& path, BigInt value);

    bool getNextNode(TreePath& path, Int level = 0, bool down = false) const
    {
        Int idx = path[level].node()->children_count();
        return getNextNode(path, level, idx, down ? 0 : level );
    }

    bool getPrevNode(TreePath& path, Int level = 0, bool down = false) const
    {
        return getPrevNode(path, level, -1, down ? 0 : level);
    }

    void finishPathStep(TreePath& path, Int key_idx) const {}

    template <typename Node>
    bool checkNodeContent(Node *node);


    template <typename Node1, typename Node2>
    bool checkNodeWithParentContent(Node1 *node, Node2 *parent, Int parent_idx);



    template <typename Node>
    Accumulator sumKeysFn(const Node* node, Int from, Int count) const
    {
    	Accumulator keys;
    	node->map().sum(from, from + count, keys);
    	return keys;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(SumKeysFn, sumKeysFn, Accumulator);




    template <typename Node>
    Key sumKeysInOneBlockFn(const Node* node, Int block_num, Int from, Int count) const
    {
    	Key keys = 0;
    	node->map().sum(block_num, from, from + count, keys);
    	return keys;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(SumKeysInOneBlockFn, sumKeysInOneBlockFn, Key);



    template <typename Node>
    void addKeysFn(Node* node, Int idx, const Key* keys, bool reindex_fully) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		node->map().updateUp(c, idx, keys[c]);
    	}

    	if (reindex_fully)
    	{
    		node->map().reindex();
    	}
    }

    MEMORIA_CONST_FN_WRAPPER(AddKeysFn, addKeysFn);



    void sumKeys(const NodeBase *node, Int from, Int count, Accumulator& keys) const
    {
    	keys += NodeDispatcher::dispatchConstRtn(node, SumKeysFn(me()), from, count);
    }

    void sumKeys(const NodeBase *node, Int block_num, Int from, Int count, Key& sum) const
    {
    	sum += NodeDispatcher::dispatchConstRtn(node, SumKeysInOneBlockFn(me()), block_num, from, count);
    }

    void addKeys(NodeBaseG& node, int idx, const Accumulator& keys, bool reindex_fully = false) const
    {
    	node.update();
    	NodeDispatcher::dispatch(node, AddKeysFn(me()), idx, keys.keys(), reindex_fully);
    }

    bool updateCounters(NodeBaseG& node, Int idx, const Accumulator& counters, bool reindex_fully = false) const;


private:

    bool getNextNode(TreePath& path, Int level, Int idx, Int target_level) const;
    bool getPrevNode(TreePath& path, Int level, Int idx, Int target_level) const;

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::ToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
BigInt M_TYPE::getTotalKeyCount() const
{
    return me()->getRootMetadata().key_count();
}

M_PARAMS
void M_TYPE::setTotalKeyCount(BigInt value)
{
    Metadata meta = me()->getRootMetadata();
    meta.key_count() = value;

    me()->setRootMetadata(meta);
}

M_PARAMS
void M_TYPE::addTotalKeyCount(BigInt value)
{
    Metadata meta       = me()->getRootMetadata();
    meta.key_count()    += value;

    me()->setRootMetadata(meta);
}


M_PARAMS
void M_TYPE::addTotalKeyCount(TreePath& path, BigInt value)
{
    NodeBaseG& node     = path[path.getSize() - 1].node();

    Metadata meta       = me()->getRootMetadata();
    meta.key_count()    += value;

    me()->setRootMetadata(node, meta);
}





M_PARAMS
bool M_TYPE::getNextNode(TreePath& path, Int level, Int idx, Int target_level) const
{
    NodeBaseG& page = path[level].node();

    if (idx < page->children_count())
    {
        for(; level != target_level && level > 0; level--)
        {
            path[level - 1].node()          = me()->getChild(path[level].node(), idx, Allocator::READ);
            path[level - 1].parent_idx()    = idx;

            idx = 0;
        }

        if (level == 0)
        {
            me()->finishPathStep(path, idx);
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
    NodeBaseG& page = path[level].node();

    if (idx >= 0)
    {
        for(; level != target_level && level > 0; level--)
        {
            path[level - 1].node()          = me()->getChild(path[level].node(), idx, Allocator::READ);
            path[level - 1].parent_idx()    = idx;

            idx = path[level - 1].node()->children_count() - 1;
        }

        if (level == 0)
        {
            me()->finishPathStep(path, idx);
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
bool M_TYPE::updateCounters(NodeBaseG& node, Int idx, const Accumulator& counters, bool reindex_fully) const
{
    node.update();
    me()->addKeys(node, idx, counters.keys(), reindex_fully);

    return false; //proceed further unconditionally
}



M_PARAMS
template <typename Node>
bool M_TYPE::checkNodeContent(Node *node) {
    bool errors = false;

    for (Int i = 0; i < Indexes; i++) {
        Key key = 0;

        for (Int c = 0; c < node->children_count(); c++) {
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
