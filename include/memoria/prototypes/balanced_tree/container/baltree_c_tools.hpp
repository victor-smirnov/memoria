
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


    struct BTreeNodeTraits {
        typedef enum {MAX_CHILDREN} Enum;
    };


    struct GetNodeTraintsFn {
        typename BTreeNodeTraits::Enum trait_;
        Int value_;
        Int page_size_;

        GetNodeTraintsFn(typename BTreeNodeTraits::Enum trait, Int page_size): trait_(trait), page_size_(page_size) {}

        template <typename Node>
        void operator()()
        {
            switch (trait_)
            {
                case BTreeNodeTraits::MAX_CHILDREN:
                    value_ = Node::Map::max_tree_size(page_size_ - sizeof(Node) + sizeof (typename Node::Map)); break;

                default: throw DispatchException(MEMORIA_SOURCE, "Unknown static node trait value", trait_);
            }
        };
    };

    Int getNodeTraitInt(typename BTreeNodeTraits::Enum trait, bool root, bool leaf, Int level) const
    {
        Int page_size = me()->getRootMetadata().page_size();
        GetNodeTraintsFn fn(trait, page_size);
        NodeDispatcher::DispatchStatic(root, leaf, level, fn);
        return fn.value_;
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

    void root2Node(NodeBaseG& node)
    {
        node.update();

        balanced_tree::Root2NodeFn<Root2NodeMap, Allocator> fn;
        RootDispatcher::Dispatch(node, fn);
    }

    void node2Root(NodeBaseG& node, Metadata& meta)
    {
        node.update();

        balanced_tree::Node2RootFn<Node2RootMap, Allocator, Metadata> fn(meta);
        NonRootDispatcher::Dispatch(node, fn);
    }

    void copyRootMetadata(NodeBaseG& src, NodeBaseG& tgt)
    {
        tgt.update();
        memoria::balanced_tree::copyRootMetadata<RootDispatcher>(src.page(), tgt.page());
    }

    template <typename TypeMap>
    class CanConvertToRootFn {
        bool can_;

    public:
        template <typename T>
        void operator()(T *node)
        {
            typedef typename memoria::Type2TypeMap<T, TypeMap, void>::Result RootType;

            Int node_children_count = node->children_count();

            Int root_block_size 	= node->page_size() - sizeof(RootType) + sizeof(typename RootType::Map);

            Int root_children_count = RootType::Map::max_tree_size(root_block_size);

            can_ = node_children_count <= root_children_count;
        }

        bool can() const {
            return can_;
        };
    };

    bool canConvertToRoot(NodeBase* node) const
    {
        CanConvertToRootFn<Node2RootMap> fn;
        NonRootDispatcher::Dispatch(node, fn);
        return fn.can();
    }

    template <typename Idx>
    class GetPageIdFn {
        const ID *id_;
        Idx idx_;
    public:
        GetPageIdFn(Idx idx): idx_(idx) {}

        template <typename T>
        void operator()(T *node) {
            id_ = &node->map().data(idx_);
        }

        const ID *id() const {
            return id_;
        };
    };

    ID getPageId(const NodeBaseG& node, Int idx) const
    {
        GetPageIdFn<Int> fn(idx);
        NonLeafDispatcher::DispatchConst(node, fn);
        return *fn.id();
    }


    NodeBaseG getChild(const NodeBase *node, Int idx, Int flags) const
    {
        return memoria::btree::getChild<NonLeafDispatcher, NodeBaseG>(node, idx, me()->allocator(), flags);
    }

    NodeBaseG getLastChild(const NodeBase *node, Int flags)
    {
        return memoria::btree::getLastChild<NonLeafDispatcher, NodeBaseG>(node, me()->allocator(), flags);
    }


    TreePathItem& getParent(TreePath& path, const NodeBaseG& node) const
    {
        return path[node->level() + 1];
    }

    template <typename Max>
    class GetCapacityFn {
        Int cap_;
        Max max_node_capacity_;
    public:
        GetCapacityFn(Max max_node_capacity): max_node_capacity_(max_node_capacity) {}

        template <typename T>
        void operator()(T *node)
        {
            if (max_node_capacity_ == 0)
            {
                cap_ = (node->map().maxSize() - node->children_count());
            }
            else
            {
                Int capacity = max_node_capacity_ - node->children_count();
                cap_ = capacity > 0 ? capacity : 0;
            }
        }

        Int cap() const {
            return cap_;
        }
    };

    Int getCapacity(const NodeBaseG& node) const
    {
        GetCapacityFn<Int> fn(me()->getBranchingFactor());
        NodeDispatcher::DispatchConst(node, fn);
        return fn.cap();
    }

    template <typename Max>
    class getMaxCapacityFn {
        Int cap_;
        Max max_node_capacity_;
    public:
        getMaxCapacityFn(Max max_node_capacity): max_node_capacity_(max_node_capacity) {}

        template <typename T>
        void operator()(T *node) {
            if (max_node_capacity_ == 0)
            {
                cap_ = node->map().maxSize();
            }
            else
            {
                cap_ = max_node_capacity_;
            }
        }

        Int cap() const {
            return cap_;
        }
    };

    Int getMaxCapacity(const NodeBaseG& node) const
    {
        getMaxCapacityFn<Int> fn(me()->getBranchingFactor());
        NodeDispatcher::DispatchConst(node, fn);
        return fn.cap();
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



    Key getKey(const NodeBaseG& node, Int i, Int idx) const
    {
        return memoria::balanced_tree::getKey<NodeDispatcher, Key>(node.page(), i, idx);
    }

    Accumulator getKeys(const NodeBaseG& node, Int idx) const
    {
        Accumulator keys;
        memoria::balanced_tree::getKeys<NodeDispatcher, Indexes, Key>(node.page(), idx, keys.keys());
        return keys;
    }

    /**
     * \brief Get maximal key value in the page.
     */

    Accumulator getMaxKeys(const NodeBaseG& node) const
    {
        Accumulator keys;
        memoria::balanced_tree::getMaxKeys<NodeDispatcher, Indexes, Key>(node.page(), keys.keys());
        return keys;
    }

    NodeBaseG getRoot(Int flags) const
    {
        return me()->allocator().getPage(me()->root(), flags);
    }

    void setKeys(NodeBaseG& node, Int idx, const Accumulator& keys) const
    {
        node.update();
        memoria::balanced_tree::setKeys<NodeDispatcher>(node.page(), idx, keys.keys());
    }

    void setChildrenCount(NodeBaseG& node, Int count) const
    {
        node.update();
        memoria::balanced_tree::setChildrenCount<NodeDispatcher>(node.page(), count);
    }

    void addChildrenCount(NodeBaseG& node, Int count) const
    {
        node.update();
        memoria::balanced_tree::addChildrenCount<NodeDispatcher>(node.page(), count);
    }

    ID getINodeData(const NodeBaseG& node, Int idx) const
    {
        return *memoria::btree::getValue<NonLeafDispatcher, ID>(node.page(), idx);
    }

    void setINodeData(NodeBaseG& node, Int idx, const ID *id) const
    {
        node.update();
        memoria::balanced_tree::setData<NonLeafDispatcher>(node.page(), idx, id);
    }

    void reindex(NodeBaseG& node) const
    {
        node.update();
        memoria::balanced_tree::Reindex<NodeDispatcher>(node.page(), 0, node->children_count());
    }

    void reindexRegion(NodeBaseG& node, Int from, Int to) const
    {
        node.update();
        memoria::balanced_tree::Reindex<NodeDispatcher>(node.page(), from, to);
    }


    class SetAndReindexFn {
        Int             i_;
        const Element&  element_;
        const MyType*   map_;
    public:
        SetAndReindexFn(Int i, const Element& element, const MyType* map): i_(i), element_(element), map_(map) {}

        template <typename T>
        void operator()(T *node)
        {
            map_->setNodeKeyValue(node, i_, element_);

            if (i_ == node->children_count() - 1)
            {
                node->map().reindexAll(i_, i_ + 1);
            }
            else {
                node->map().reindexAll(i_, node->children_count());
            }
        }
    };

    template <typename Node>
    void setNodeKeyValue(Node* node, Int idx, const Element& element) const
    {
        for (Int c = 0; c < MyType::Indexes; c++)
        {
            node->map().key(c, idx) = element.first[c];
        }

        node->map().data(idx) = element.second;
    }

    void setLeafDataAndReindex(NodeBaseG& node, Int idx, const Element& element) const
    {
        node.update();
        SetAndReindexFn fn(idx, element, me());
        LeafDispatcher::Dispatch(node.page(), fn);
    }
    
    Value getLeafData(const NodeBaseG& node, Int idx) const
    {
        return *memoria::btree::getValue<LeafDispatcher, Value>(node.page(), idx);
    }

    void setLeafData(NodeBaseG& node, Int idx, const Value &val) const
    {
        node.update();
        memoria::btree::setData<LeafDispatcher>(node.page(), idx, &val);
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


    //FIXME: SumKeysFn is used in MoveElementsFn
    struct SumKeysFn {

    	Int             from_;
    	Int             count_;
    	Accumulator&    keys_;

    public:
    	SumKeysFn(Int from, Int count, Accumulator& keys):
    		from_(from), count_(count), keys_(keys) {}

    	template <typename T>
    	void operator()(T *node)
    	{
    		node->map().sum(from_, from_ + count_, keys_);
    	}
    };

private:


    struct SumKeysInOneBlockFn {

    	Int             from_;
    	Int             count_;
    	Key&            sum_;
    	Int             block_num_;

    public:
    	SumKeysInOneBlockFn(Int from, Int count, Key& sum, Int block_num):
    		from_(from), count_(count), sum_(sum), block_num_(block_num) {}

    	template <typename T>
    	void operator()(T *node)
    	{
    		node->map().sum(block_num_, from_, from_ + count_, sum_);
    	}
    };

    struct AddKeysFn {
    	Int idx_;
    	const Key *keys_;
    	bool reindex_fully_;

    	AddKeysFn(Int idx, const Key* keys, bool reindex_fully): idx_(idx), keys_(keys), reindex_fully_(reindex_fully) {}

    	template <typename Node>
    	void operator()(Node *node)
    	{
    		for (Int c = 0; c < Indexes; c++)
    		{
    			node->map().updateUp(c, idx_, keys_[c]);
    		}

    		if (reindex_fully_) {
    			node->map().reindex();
    		}
    	}
    };

public:

    void sumKeys(const NodeBase *node, Int from, Int count, Accumulator& keys) const
    {
    	SumKeysFn fn(from, count, keys);
    	NodeDispatcher::DispatchConst(node, fn);
    }

    void sumKeys(const NodeBase *node, Int block_num, Int from, Int count, Key& sum) const
    {
    	SumKeysInOneBlockFn fn(from, count, sum, block_num);
    	NodeDispatcher::DispatchConst(node, fn);
    }

    void addKeys(NodeBaseG& node, int idx, const Accumulator& keys, bool reindex_fully = false) const
    {
    	node.update();

    	AddKeysFn fn(idx, keys.keys(), reindex_fully);
    	NodeDispatcher::Dispatch(node, fn);
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
