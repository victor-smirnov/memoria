
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_TOOLS_HPP

#include <iostream>

#include <memoria/prototypes/btree/pages/pages.hpp>

#include <memoria/metadata/tools.hpp>


namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_NO_CTR_BEGIN(memoria::btree::ToolsName)
private:
    Int max_node_capacity_;
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Allocator::PageG                                     PageG;

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Base::NodeBase                                            	NodeBase;
    typedef typename Base::NodeBaseG                                           	NodeBaseG;

    typedef typename Base::NodeBase::Base                                      	TreeNodePage;

    typedef typename Base::NodeDispatcher                              			NodeDispatcher;
    typedef typename Base::RootDispatcher                               		RootDispatcher;
    typedef typename Base::LeafDispatcher                               		LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                            		NonLeafDispatcher;
    typedef typename Base::NonRootDispatcher                            		NonRootDispatcher;

    typedef typename Base::Node2RootMap                                 		Node2RootMap;
    typedef typename Base::Root2NodeMap                                		 	Root2NodeMap;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;
    typedef typename Base::Accumulator											Accumulator;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int Indexes                                                    = Types::Indexes;

    CtrPart(): Base(), max_node_capacity_(-1) {}
    CtrPart(const ThisType& other): Base(other), max_node_capacity_(other.max_node_capacity_) 		{}
    CtrPart(ThisType&& other): Base(std::move(other)), max_node_capacity_(other.max_node_capacity_) {}


    CtrPart(ThisType&& other, Allocator& allocator): Base(std::move(other), allocator)  {}
    CtrPart(const ThisType& other, Allocator& allocator): Base(other, allocator)  		{}


    struct BTreeNodeTraits {
    	typedef enum {MAX_CHILDREN} Enum;
    };


    struct GetNodeTraintsFn {
    	typename BTreeNodeTraits::Enum trait_;
    	Int value_;

    	GetNodeTraintsFn(typename BTreeNodeTraits::Enum trait): trait_(trait) {}

    	template <typename Node>
    	void operator()()
    	{
    		switch (trait_)
    		{
    			case BTreeNodeTraits::MAX_CHILDREN: value_ = Node::Map::max_size(); break;

    			default: throw DispatchException(MEMORIA_SOURCE, "Unknown static node trait value", trait_);
    		}
    	};
    };

    Int GetNodeTraitInt(typename BTreeNodeTraits::Enum trait, bool root, bool leaf, Int level) const
    {
    	GetNodeTraintsFn fn(trait);
    	NodeDispatcher::DispatchStatic(root, leaf, level, fn);
    	return fn.value_;
    }

    Int GetMaxKeyCountForNode(bool root, bool leaf, Int level) const
    {
    	Int key_count = GetNodeTraitInt(BTreeNodeTraits::MAX_CHILDREN, root, leaf, level);
    	Int max_count = me()->GetMaxChildrenPerNode();

    	if (max_count == -1)
    	{
    		return key_count;
    	}
    	else {
    		return key_count < max_count? key_count : max_count;
    	}
    }

    bool IsTheSameNode(const TreePath& path1, const TreePath& path2, int level) const
    {
    	return path1[level].node()->id() == path2[level].node()->id();
    }

    void operator=(ThisType&& other)
    {
    	this->max_node_capacity_ = other.max_node_capacity_;
    	Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other)
    {
    	this->max_node_capacity_ = other.max_node_capacity_;
    	Base::operator=(other);
    }

    void SetMaxChildrenPerNode(Int count) {
        max_node_capacity_ = count;
    }

    Int GetMaxChildrenPerNode() const {
        return max_node_capacity_;
    }

    Int &max_node_capacity() {
        return max_node_capacity_;
    }

    const Int &max_node_capacity() const {
        return max_node_capacity_;
    }

    void Root2Node(NodeBaseG& node)
    {
        node.update();
    	node.set_page(memoria::btree::Root2Node<RootDispatcher, Root2NodeMap, Allocator>(node.page()));
    }

    void Node2Root(NodeBaseG& node, Metadata& meta)
    {
    	node.update();

        node.set_page(memoria::btree::Node2Root<NonRootDispatcher, Node2RootMap, Allocator>(node.page()));

        me()->SetRootMetadata(node, meta);
    }

    void CopyRootMetadata(NodeBaseG& src, NodeBaseG& tgt)
    {
        tgt.update();
    	memoria::btree::CopyRootMetadata<RootDispatcher>(src.page(), tgt.page());
    }

    template <typename TypeMap>
    class CanConvertToRootFn {
        bool can_;

    public:
        template <typename T>
        void operator()(T *node)
        {
            typedef typename memoria::Type2TypeMap<T, TypeMap, void>::Result RootType;
            can_ = node->children_count() <= RootType::Map::max_size();
        }

        bool can() const {
            return can_;
        };
    };

    bool CanConvertToRoot(NodeBase* node) const
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

    ID GetPageId(const NodeBaseG& node, Int idx) const
    {
        GetPageIdFn<Int> fn(idx);
        NonLeafDispatcher::DispatchConst(node, fn);
        return *fn.id();
    }


    NodeBaseG GetChild(const NodeBase *node, Int idx, Int flags) const
    {
        return memoria::btree::GetChild<NonLeafDispatcher, NodeBaseG>(node, idx, me()->allocator(), flags);
    }

    NodeBaseG GetLastChild(const NodeBase *node, Int flags)
    {
        return memoria::btree::GetLastChild<NonLeafDispatcher, NodeBaseG>(node, me()->allocator(), flags);
    }


    TreePathItem& GetParent(TreePath& path, const NodeBaseG& node) const
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
            if (max_node_capacity_ == -1)
            {
                cap_ = (node->map().max_size() - node->children_count());
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

    Int GetCapacity(const NodeBaseG& node) const
    {
        GetCapacityFn<Int> fn(me()->max_node_capacity());
        NodeDispatcher::DispatchConst(node, fn);
        return fn.cap();
    }

    template <typename Max>
    class GetMaxCapacityFn {
        Int cap_;
        Max max_node_capacity_;
    public:
        GetMaxCapacityFn(Max max_node_capacity): max_node_capacity_(max_node_capacity) {}

        template <typename T>
        void operator()(T *node) {
            if (max_node_capacity_ == -1)
            {
                cap_ = node->map().max_size();
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

    Int GetMaxCapacity(const NodeBaseG& node) const
    {
        GetMaxCapacityFn<Int> fn(me()->max_node_capacity());
        NodeDispatcher::DispatchConst(node, fn);
        return fn.cap();
    }

    bool ShouldMergeNode(const TreePath& path, Int level) const
    {
    	const NodeBaseG& node = path[level].node();
    	return node->children_count() <= me()->GetMaxCapacity(node) / 2;
    }

    bool ShouldSplitNode(const TreePath& path, Int level) const
    {
    	const NodeBaseG& node = path[level].node();
    	return me()->GetCapacity(node) == 0;
    }

    struct GetMetadataFn {
        Metadata metadata_;
    
        GetMetadataFn() {}

        template <typename T>
        void operator()(T *node) {
            metadata_ = node->metadata();
        }
    };


    struct SetMetadataFn {
    	Metadata metadata_;

    	SetMetadataFn(const Metadata& metadata): metadata_(metadata) {}

    	template <typename T>
    	void operator()(T *node) {
    		node->metadata() = metadata_;
    	}
    };

    static Metadata GetCtrRootMetadata(const NodeBaseG& node)
    {
    	GetMetadataFn fn;
    	RootDispatcher::DispatchConst(node, fn);
    	return fn.metadata_;
    }

    Metadata GetRootMetadata(const NodeBaseG& node) const
    {
    	return GetCtrRootMetadata(node);
    }



    Metadata GetRootMetadata() const
    {
    	NodeBaseG root = me()->GetRoot(Allocator::READ);
    	return me()->GetRootMetadata(root);
    }

    void SetRootMetadata(const Metadata& metadata)
    {
    	NodeBaseG root = me()->GetRoot(Allocator::UPDATE);
    	me()->SetRootMetadata(root, metadata);
    }

    void SetRootMetadata(NodeBaseG& node, const Metadata& metadata)
    {
    	node.update();
    	SetMetadataFn fn(metadata);
    	RootDispatcher::Dispatch(node, fn);
    }

    BigInt GetContainerName(const NodeBaseG& node) const
    {
        return GetRootMetadata(node).model_name();
    }

    struct SumKeysFn {

        Int         from_;
        Int         count_;
        Key*        keys_;

    public:
        SumKeysFn(Int from, Int count, Key* keys):
                            from_(from), count_(count), keys_(keys) {}

        template <typename T>
        void operator()(T *node)
        {
            for (Int d = 0; d < Indexes; d++)
            {
                for (Int c = from_; c < from_ + count_; c++)
                {
                    keys_[d] += node->map().key(d, c);
                }
            }
        }
    };

    void SumKeys(const NodeBase *node, Int from, Int count, Key* keys) const
    {
        SumKeysFn fn(from, count, keys);
        NodeDispatcher::DispatchConst(node, fn);
    }

    struct AddKeysFn {
        Int idx_;
        const Key *keys_;

        AddKeysFn(Int idx, const Key* keys): idx_(idx), keys_(keys) {}

        template <typename Node>
        void operator()(Node *node)
        {
            for (Int c = 0; c < Indexes; c++)
            {
            	node->map().key(c, idx_) += keys_[c];
            }

            node->map().Reindex();
        }
    };

    void AddKeys(NodeBaseG& node, int idx, const Accumulator& keys) const
    {
        node.update();

        AddKeysFn fn(idx, keys.keys());
        NodeDispatcher::Dispatch(node, fn);
    }

    Key GetKey(const NodeBaseG& node, Int i, Int idx) const
    {
        return memoria::btree::GetKey<NodeDispatcher, Key>(node.page(), i, idx);
    }

    Accumulator GetKeys(const NodeBaseG& node, Int idx) const
    {
        Accumulator keys;
    	memoria::btree::GetKeys<NodeDispatcher, Indexes, Key>(node.page(), idx, keys.keys());
    	return keys;
    }

    Accumulator GetMaxKeys(const NodeBaseG& node) const
    {
    	Accumulator keys;
    	memoria::btree::GetMaxKeys<NodeDispatcher, Indexes, Key>(node.page(), keys.keys());
    	return keys;
    }

    NodeBaseG GetRoot(Int flags) const
    {
    	return me()->allocator().GetPage(me()->root(), flags);
    }

    void SetKeys(NodeBaseG& node, Int idx, const Accumulator& keys) const
    {
    	node.update();
    	memoria::btree::SetKeys<NodeDispatcher>(node.page(), idx, keys.keys());
    }

    void SetChildrenCount(NodeBaseG& node, Int count) const
    {
        node.update();
    	memoria::btree::SetChildrenCount<NodeDispatcher>(node.page(), count);
    }

    void AddChildrenCount(NodeBaseG& node, Int count) const
    {
    	node.update();
    	memoria::btree::AddChildrenCount<NodeDispatcher>(node.page(), count);
    }

    ID GetINodeData(const NodeBaseG& node, Int idx) const
    {
    	return *memoria::btree::GetValue<NonLeafDispatcher, ID>(node.page(), idx);
    }

    void SetINodeData(NodeBaseG& node, Int idx, const ID *id) const
    {
    	node.update();
    	memoria::btree::SetData<NonLeafDispatcher>(node.page(), idx, id);
    }

    void Reindex(NodeBaseG& node) const
    {
        node.update();
    	memoria::btree::Reindex<NodeDispatcher>(node.page());
    }


    class SetAndReindexFn {
        Int       		i_;
        const Element& 	element_;
        const MyType* 	map_;
    public:
        SetAndReindexFn(Int i, const Element& element, const MyType* map): i_(i), element_(element), map_(map) {}

        template <typename T>
        void operator()(T *node)
        {
            map_->SetNodeKeyValue(node, i_, element_);
        	node->map().Reindex();
        }
    };

    template <typename Node>
    void SetNodeKeyValue(Node* node, Int idx, const Element& element) const
    {
        for (Int c = 0; c < MyType::Indexes; c++)
        {
            node->map().key(c, idx) = element.first[c];
        }

        node->map().data(idx) = element.second;
    }

    void SetLeafDataAndReindex(NodeBaseG& node, Int idx, const Element& element) const
    {
        node.update();
    	SetAndReindexFn fn(idx, element, me());
    	LeafDispatcher::Dispatch(node.page(), fn);
    }
    
    Value GetLeafData(const NodeBaseG& node, Int idx) const
    {
        return *memoria::btree::GetValue<LeafDispatcher, Value>(node.page(), idx);
    }

    void SetLeafData(NodeBaseG& node, Int idx, const Value &val) const
    {
        node.update();
    	memoria::btree::SetData<LeafDispatcher>(node.page(), idx, &val);
    }

    void Dump(PageG page, std::ostream& out = std::cout) const
    {
    	if (page != NULL)
    	{
    		PageWrapper<Page, Allocator::PAGE_SIZE> pw(page);
    		PageMetadata* meta = me()->reflection()->GetPageMetadata(pw.GetPageTypeHash());
    		memoria::vapi::DumpPage(meta, &pw, out);
    		out<<endl;
    		out<<endl;
    	}
    	else {
    		out<<"NULL"<<std::endl;
    	}
    }

    BigInt GetTotalKeyCount() const;
    void SetTotalKeyCount(BigInt value);
    void AddTotalKeyCount(BigInt value);
    void AddTotalKeyCount(TreePath& path, BigInt value);

    bool GetNextNode(TreePath& path, Int level = 0, bool down = false) const
    {
    	Int idx = path[level].node()->children_count();
    	return GetNextNode(path, level, idx, down ? 0 : level );
    }

    bool GetPrevNode(TreePath& path, Int level = 0, bool down = false) const
    {
    	return GetPrevNode(path, level, -1, down ? 0 : level);
    }

    void FinishPathStep(TreePath& path, Int key_idx) const {}

private:

    bool GetNextNode(TreePath& path, Int level, Int idx, Int target_level) const;
    bool GetPrevNode(TreePath& path, Int level, Int idx, Int target_level) const;

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::ToolsName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
BigInt M_TYPE::GetTotalKeyCount() const
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);

	if (node.is_set())
	{
		Metadata meta = me()->GetRootMetadata(node);
		return meta.key_count();
	}
	else {
		return 0;
	}
}

M_PARAMS
void M_TYPE::SetTotalKeyCount(BigInt value)
{
	NodeBaseG node = me()->GetRoot(Allocator::UPDATE);
	if (node.is_set())
	{
		Metadata meta = me()->GetRootMetadata(node);
		meta.key_count() = value;

		me()->SetRootMetadata(node, meta);
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, String("Root node is not set for this container: ") + me()->type_name());
	}
}

M_PARAMS
void M_TYPE::AddTotalKeyCount(BigInt value)
{
	NodeBaseG node = me()->GetRoot(Allocator::UPDATE);
	if (node.is_set())
	{
		Metadata meta = me()->GetRootMetadata(node);
		meta.key_count() += value;

		me()->SetRootMetadata(node, meta);
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, String("Root node is not set for this container: ") + me()->type_name());
	}
}


M_PARAMS
void M_TYPE::AddTotalKeyCount(TreePath& path, BigInt value)
{
	NodeBaseG& node = path[path.GetSize() - 1].node();

	Metadata meta = me()->GetRootMetadata(node);

	meta.key_count() += value;

	me()->SetRootMetadata(node, meta);
}





M_PARAMS
bool M_TYPE::GetNextNode(TreePath& path, Int level, Int idx, Int target_level) const
{
	NodeBaseG& page = path[level].node();

	if (idx < page->children_count())
	{
		for(; level != target_level && level > 0; level--)
		{
			path[level - 1].node() 			= me()->GetChild(path[level].node(), idx, Allocator::READ);
			path[level - 1].parent_idx() 	= idx;

			idx = 0;
		}

		if (level == 0)
		{
			me()->FinishPathStep(path, idx);
		}
		return true;
	}
	else {
		if (!page->is_root())
		{
			return GetNextNode(path, level + 1, path[level].parent_idx() + 1, target_level);
		}
	}

	return false;
}


M_PARAMS
bool M_TYPE::GetPrevNode(TreePath& path, Int level, Int idx, Int target_level) const
{
	NodeBaseG& page = path[level].node();

	if (idx >= 0)
	{
		for(; level != target_level && level > 0; level--)
		{
			path[level - 1].node() 			= me()->GetChild(path[level].node(), idx, Allocator::READ);
			path[level - 1].parent_idx() 	= idx;

			idx = path[level - 1].node()->children_count() - 1;
		}

		if (level == 0)
		{
			me()->FinishPathStep(path, idx);
		}

		return true;
	}
	else {
		if (!page->is_root())
		{
			return GetPrevNode(path, level + 1, path[level].parent_idx() - 1, target_level);
		}
	}

	return false;
}





#undef M_TYPE
#undef M_PARAMS

}


#endif
