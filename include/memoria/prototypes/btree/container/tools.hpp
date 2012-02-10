
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
    typedef typename Page::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;

    typedef typename Types::NodeBase::Base                                      TreeNodePage;
    typedef typename Types::Counters                                            Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
    typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    static const Int Indexes                                                    = Types::Indexes;
    static const bool MapType                                                   = Types::MapType;

    CtrPart(): Base(), max_node_capacity_(-1) {}
    CtrPart(const ThisType& other): Base(other), max_node_capacity_(other.max_node_capacity_) {}
    CtrPart(ThisType&& other): Base(std::move(other)), max_node_capacity_(other.max_node_capacity_) {}

    void ClearKeys(Key* keys) const {
    	for (Int c = 0; c < Indexes; c++) keys[c] = 0;
    }

    void NegateKeys(Key* keys) const {
    	NegateKeys(keys, keys);
    }

    void NegateKeys(Key* result, Key* keys) const {
    	for (Int c = 0; c < Indexes; c++) result[c] = -keys[c];
    }

    void AddKeys(Key* sum, Key* keys) const {
    	for (Int c = 0; c < Indexes; c++) sum[c] += keys[c];
    }

    void SetKeys(Key* target, Key* keys) const {
    	for (Int c = 0; c < Indexes; c++) target[c] = keys[c];
    }

    void Dump(Key* keys, ostream& out = cout) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		out << keys[c] <<endl;
    	}
    	out<<endl;
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

    virtual void SetMaxChildrenPerNode(Int count) {
        max_node_capacity_ = count;
    }

    virtual Int GetMaxChildrenPerNode() {
        return max_node_capacity_;
    }

    Int &max_node_capacity() {
        return max_node_capacity_;
    }

    const Int &max_node_capacity() const {
        return max_node_capacity_;
    }


    template <typename Map, typename NodeBase>
    class UpdateBTreeKeysFn1 {
        Map &map_;
        NodeBaseG parent_;
        bool add_mode_;
    public:

        UpdateBTreeKeysFn1(Map &map, NodeBaseG parent, bool add_mode) :
                    map_(map), parent_(parent),
                    add_mode_(add_mode)
                    {}

        template<typename T>
        void operator()(T *node)
        {
            Key keys[Indexes];
            for (Int c = 0; c < Indexes; c++) keys[c] = node->map().max_key(c);
            
            map_.UpdateBTreeKeys(
                    parent_,
                    node->parent_idx(),
                    keys,
                    add_mode_
            );
        }
    };

    void UpdateBTreeKeys(NodeBaseG node, bool add_mode = false)
    {
        NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);
        if (parent != NULL)
        {
            UpdateBTreeKeysFn1<MyType, NodeBase> fn(*me(), parent, add_mode);
            NodeDispatcher::Dispatch(node, fn);
        }
    }


    template <typename Map, typename Keys>
    class UpdateBTreeKeysFn2 {
        bool retn_;
        NodeBaseG node_;
        Keys* keys_;
        Int idx_;

        Map &map_;
        bool add_mode_;
    public:
        UpdateBTreeKeysFn2(Map &map, Int idx, Keys *keys, bool add_mode):
                            retn_(false), node_(NULL), keys_(keys),
                            idx_(idx), map_(map),
                            add_mode_(add_mode)
        {}

        template<typename T>
        void operator()(T *node)
        {
            for (Int c = 0; c < Indexes; c++)
            {
                if (add_mode_)
                {
                	node->map().key(c, idx_) += keys_[c];
                }
                else {
                	node->map().key(c, idx_) = keys_[c];
                }
            }

            node->Reindex();

            idx_  = node->parent_idx();

            if (!add_mode_)
            {
            	for (Int c = 0; c < Indexes; c++)
            	{
            		keys_[c] = node->map().max_key(c);
            	}
            }

            node_ = map_.GetParent(node, Allocator::UPDATE);
        }

        

        NodeBaseG& node() {
            return node_;
        }

        Int idx() const {
            return idx_;
        }

        bool retn() const {
            return retn_;
        }
    };

    template <typename Map, typename Keys>
    class UpdateBTreeKeysFn3 {
        bool retn_;
        Map& map_;
        Int idx_;
        const Keys *keys_;
    public:
        UpdateBTreeKeysFn3(Map& map, Int idx, const Keys *keys): map_(map), idx_(idx), keys_(keys) {}

        template<typename T>
        void operator()(T *node)
        {
            for (Int c = 0; c < Indexes; c++)
            {
                if (node->map().key(c, idx_) != keys_[c])
                {
                    retn_ = false;
                    return;
                }
            }
            retn_ = true;
        }

        bool retn() const
        {
            return retn_;
        }
    };


    template <typename Map, typename Keys>
    class UpdateBTreeKeysFn4 {
        Map& map_;
        Int idx_;
        const Keys *keys_;
        bool add_mode_;
    public:
        UpdateBTreeKeysFn4(Map& map, Int idx, const Keys *keys, bool add_mode):
        	map_(map), idx_(idx), keys_(keys),
        	add_mode_(add_mode)
        {}

        template<typename T>
        void operator()(T *node) {
            for (Int c = 0; c < Indexes; c++)
            {
                if (add_mode_) {
                	node->map().key(c, idx_) += keys_[c];
                }
                else {
                	node->map().key(c, idx_) = keys_[c];
                }
            }
            node->Reindex();
        }
    };

    void UpdateBTreeKeys(NodeBaseG& node0, Int idx, const Key *keys, bool add_mode = false)
    {
        Key tkeys[Indexes];
        for (Int c = 0; c < Indexes; c++) tkeys[c] = keys[c];

        node0.update();
        NodeBaseG node = node0;

        while (!node->is_root())
        {
            node.update();
        	UpdateBTreeKeysFn2<MyType, Key> fn2(*me(), idx, tkeys, add_mode);
            NodeDispatcher::Dispatch(node, fn2);

            idx = fn2.idx();
            node = fn2.node();
        }

        UpdateBTreeKeysFn4<MyType, Key> fn4(*me(), idx, tkeys, add_mode);
        NodeDispatcher::Dispatch(node, fn4);
    }


    void UpdateBTreeCounters(NodeBaseG& node0, const Counters &counters)
    {
    	node0.update();
    	NodeBaseG node = node0;

    	while (!node->is_root())
        {
            node->counters().page_count() += counters.page_count();
            node->counters().key_count() += counters.key_count();

            node = me()->GetParent(node, Allocator::UPDATE);
        }

        node->counters().page_count() += counters.page_count();
        node->counters().key_count() += counters.key_count();
    }


    void Root2Node(NodeBaseG& node)
    {
        node.update();
    	node.set_page(memoria::btree::Root2Node<RootDispatcher, Root2NodeMap, Allocator>(node.page()));
    }

    //

    void Node2Root(NodeBaseG& node, Metadata& meta)
    {
    	node.update();

        node.set_page(memoria::btree::Node2Root<NonRootDispatcher, Node2RootMap, Allocator>(node.page()));

        node->parent_id().Clear();
        node->parent_idx() = 0;

//        Metadata meta = me()->GetRootMetadata(node);
//        meta.model_name() = me()->name();

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
        void operator()(T *node) {
            typedef typename memoria::Type2TypeMap<T, TypeMap, void>::Result RootType;
            can_ = node->children_count() <= RootType::Map::max_size();
        }

        bool can() const {
            return can_;
        };
    };

    bool CanConvertToRoot(NodeBase *node) const
    {
        CanConvertToRootFn<Node2RootMap> fn;
        NonRootDispatcher::Dispatch(node, fn);
        return fn.can();
    }

    template <typename Idx>
    class GetPageIdFn {
        ID *id_;
        Idx idx_;
    public:
        GetPageIdFn(Idx idx): idx_(idx) {}

        template <typename T>
        void operator()(T *node) {
            id_ = &node->map().data(idx_);
        }

        ID *id() const {
            return id_;
        };
    };

    ID& GetPageId(NodeBase *node, Int idx)
    {
        GetPageIdFn<Int> fn(idx);
        NonLeafDispatcher::Dispatch(node, fn);
        return *fn.id();
    }


    NodeBaseG GetChild(NodeBase *node, Int idx, Int flags)
    {
        return memoria::btree::GetChild<NonLeafDispatcher, NodeBaseG>(node, idx, me()->allocator(), flags);
    }

    NodeBaseG GetLastChild(NodeBase *node, Int flags)
    {
        return memoria::btree::GetLastChild<NonLeafDispatcher, NodeBaseG>(node, me()->allocator(), flags);
    }


    NodeBaseG GetParent(NodeBase *node, Int flags)
    {
        if (node->is_root())
        {
            return NodeBaseG();
        }
        else
        {
        	return me()->allocator().GetPage(node->parent_id(), flags);
        }
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

    Int GetCapacity(NodeBase *node)
    {
        GetCapacityFn<Int> fn(me()->max_node_capacity());
        NodeDispatcher::Dispatch(node, fn);
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

    Int GetMaxCapacity(NodeBase *node) const
    {
        GetMaxCapacityFn<Int> fn(me()->max_node_capacity());
        NodeDispatcher::Dispatch(node, fn);
        return fn.cap();
    }

    bool ShouldMerge(NodeBase* node) const
    {
    	return node->children_count() <= me()->GetMaxCapacity(node) / 2;
    }

    bool ShouldSplit(NodeBase* node) const
    {
    	return node->children_count() > me()->GetMaxCapacity(node) / 2;
    }

    template <bool IsGet>
    struct MetadataFn {
        Metadata metadata_;
    
        MetadataFn() {}
        MetadataFn(const Metadata& metadata): metadata_(metadata) {}

        template <typename T>
        void operator()(T *node) {
            if (IsGet) {
                metadata_ = node->metadata();
            }
            else {
                node->metadata() = metadata_;
            }
        }
    };

    static const Metadata GetRootMetadata(NodeBase *node)
    {
        MetadataFn<true> fn;
        RootDispatcher::Dispatch(node, fn);
        return fn.metadata_;
    }

    static void SetRootMetadata(NodeBaseG& node, const Metadata& metadata)
    {
        node.update();
    	MetadataFn<false> fn(metadata);
        RootDispatcher::Dispatch(node, fn);
    }

    static BigInt GetContainerName(NodeBase *node)
    {
        return GetRootMetadata(node).model_name();
    }

    static BigInt GetContainerNameFormPage(Page* page)
    {
        NodeBase* node = static_cast<NodeBase*>(page);
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

    void SumKeys(NodeBase *node, Int from, Int count, Key* keys)
    {
        SumKeysFn fn(from, count, keys);
        NodeDispatcher::Dispatch(node, fn);
    }

    struct AddKeysFn {
        Int idx_;
        const Key *keys_;

        AddKeysFn(Int idx, const Key* keys): idx_(idx), keys_(keys) {}

        template <typename Node>
        void operator()(Node *node) {
            for (Int c = 0; c < Indexes; c++)
            {
                node->map().key(c, idx_) += keys_[c];
            }
            node->map().Reindex();
        }
    };

    void AddKeys(NodeBaseG& node, int idx, const Key* keys, bool deep = true)
    {
        node.update();

        AddKeysFn fn(idx, keys);
        NodeDispatcher::Dispatch(node, fn);

        if (deep && !node->is_leaf())
        {
        	NodeBaseG child = me()->GetChild(node, idx, Allocator::UPDATE);
            AddKeys(child, 0, keys);
        }
    }

    void AddKeysUp(NodeBaseG& node, int idx, const Key* keys)
    {
    	AddKeys(node, idx, keys, false);

    	if (!node->is_root())
    	{
    		NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);
    		Int parent_idx = node->parent_idx();

    		AddKeysUp(parent, parent_idx, keys);
    	}
    }


    NodeBaseG GetNode(ID &id, Int flags)
    {
        return me()->allocator().GetPage(id, flags);
    }

    static Key GetKey(NodeBase *node, Int i, Int idx)
    {
        return memoria::btree::GetKey<NodeDispatcher, Key>(node, i, idx);
    }


    static void GetKeys(NodeBase *node, Int idx, Key* keys)
    {
        memoria::btree::GetKeys<NodeDispatcher, Indexes, Key>(node, idx, keys);
    }

    static void GetMaxKeys(NodeBase *node, Key* keys)
    {
        memoria::btree::GetMaxKeys<NodeDispatcher, Indexes, Key>(node, keys);
    }

    static Key GetMaxKey(NodeBase *node, Int i)
    {
        return memoria::btree::GetMaxKey<NodeDispatcher, Key>(node, i);
    }

    NodeBaseG GetRoot(Int flags) const
    {
        return me()->allocator().GetPage(me()->root(), flags);
    }

    void SetKeys(NodeBaseG& node, Int idx, const Key *keys)
    {
    	node.update();
    	memoria::btree::SetKeys<NodeDispatcher>(node.page(), idx, keys);
    }

    void SetChildrenCount(NodeBaseG& node, Int count)
    {
        node.update();
    	memoria::btree::SetChildrenCount<NodeDispatcher>(node.page(), count);
    }

    void AddChildrenCount(NodeBaseG& node, Int count)
    {
    	node.update();
    	memoria::btree::AddChildrenCount<NodeDispatcher>(node.page(), count);
    }

    void SetINodeData(NodeBaseG& node, Int idx, const ID *id)
    {
    	node.update();
    	memoria::btree::SetData<NonLeafDispatcher>(node.page(), idx, id);
    }

    void Reindex(NodeBaseG& node)
    {
        node.update();
    	memoria::btree::Reindex<NodeDispatcher>(node.page());
    }

    void SetLeafDataAndReindex(NodeBaseG& node, Int idx, const Key *keys, const Value &val)
    {
        node.update();
    	memoria::btree::SetKeyDataAndReindex<LeafDispatcher>(node.page(), idx, keys, &val);
    }
    
    static Value GetLeafData(NodeBase *node, Int idx)
    {
        return *memoria::btree::GetData<LeafDispatcher, Value>(node, idx);
    }

    void SetLeafData(NodeBaseG& node, Int idx, const Value &val)
    {
        node.update();
    	memoria::btree::SetData<LeafDispatcher>(node.page(), idx, &val);
    }

    void Dump(PageG page, std::ostream& out = std::cout)
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

    BigInt GetPageCount()
    {
    	NodeBaseG root = me()->GetRoot(Allocator::READ);
    	return root->counters().page_count();
    }

    BigInt GetKeyCount()
    {
    	NodeBaseG root = me()->GetRoot(Allocator::READ);
    	return root->counters().key_count();
    }

MEMORIA_CONTAINER_PART_END

}


#endif	/* _MEMORIA_PROTOTYPES_BTREE_MODEL_TOOLS_HPP */
