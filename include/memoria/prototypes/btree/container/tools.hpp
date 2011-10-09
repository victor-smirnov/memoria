
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_TOOLS_HPP

#include <iostream>

#include <memoria/prototypes/btree/pages/pages.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_NO_CTR_BEGIN(memoria::btree::ToolsName)
private:
    Int max_node_capacity_;
public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                              Allocator;

    typedef typename Allocator::Page                                              Page;
    typedef typename Page::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
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

    CtrPart(MyType &me): Base(me), me_(me) {
        max_node_capacity_ = -1;
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
        NodeBase *parent_;
        bool add_mode_;
    public:

        UpdateBTreeKeysFn1(Map &map, NodeBase *parent, bool add_mode) :
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

    void UpdateBTreeKeys(NodeBase *node, bool add_mode = false)
    {
        NodeBase* parent = me_.GetParent(node);
        if (parent != NULL)
        {
            UpdateBTreeKeysFn1<MyType, NodeBase> fn(me_, parent, add_mode);
            NodeDispatcher::Dispatch(node, fn);
        }
    }


    template <typename Map, typename Keys>
    class UpdateBTreeKeysFn2 {
        bool retn_;
        NodeBase *node_;
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

            node_ = map_.GetParent(node);
        }

        

        NodeBase *node() const {
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

    void UpdateBTreeKeys(NodeBase *node, Int idx, const Key *keys, bool add_mode = false)
    {
    	MEMORIA_TRACE(me_, "[node.id, idx, keys[0]", node->id(), idx, keys[0], add_mode);

        Key tkeys[Indexes];
        for (Int c = 0; c < Indexes; c++) tkeys[c] = keys[c];

        while (!node->is_root())
        {
            UpdateBTreeKeysFn2<MyType, Key> fn2(me_, idx, tkeys, add_mode);
            NodeDispatcher::Dispatch(node, fn2);
            
            UpdateBTreeKeysFn3<MyType, Key> fn3(me_, fn2.idx(), tkeys);
            NodeDispatcher::Dispatch(fn2.node(), fn3);
            
            if (fn3.retn())
            {
                return;
            }
            else {
                idx = fn2.idx();
                node = fn2.node();
            }
        }

        UpdateBTreeKeysFn4<MyType, Key> fn4(me_, idx, tkeys, add_mode);
        NodeDispatcher::Dispatch(node, fn4);
    }


    void UpdateBTreeCounters(NodeBase *node, const Counters &counters) {
        while (!node->is_root())
        {
            node->counters().page_count() += counters.page_count();
            node->counters().key_count() += counters.key_count();

            node = me_.GetParent(node);
        }

        node->counters().page_count() += counters.page_count();
        node->counters().key_count() += counters.key_count();
    }


    NodeBase *Root2Node(NodeBase *node)
    {
        return memoria::btree::Root2Node<RootDispatcher, Root2NodeMap, Allocator>(node);
    }

    NodeBase *Node2Root(NodeBase *node)
    {
        NodeBase* root = memoria::btree::Node2Root<NonRootDispatcher, Node2RootMap, Allocator>(node);
        root->parent_id().Clear();
        root->parent_idx() = 0;

        Metadata meta = me_.GetRootMetadata(node);
        meta.model_name() = me_.name();
        me_.SetRootMetadata(node, meta);
        
        return root;
    }

    void CopyRootMetadata(NodeBase *src, NodeBase *tgt)
    {
        memoria::btree::CopyRootMetadata<RootDispatcher>(src, tgt);
    }

    template <typename TypeMap>
    class CanConvertToRootFn {
        bool can_;

    public:
        template <typename T>
        void operator()(T *node) {
            typedef typename memoria::Type2TypeMap<T, TypeMap, void>::Result RootType;
            can_ = node->map().size() <= RootType::Map::max_size();
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


    NodeBase* GetChild(NodeBase *node, Int idx)
    {
        return memoria::btree::GetChild<NonLeafDispatcher, NodeBase>(node, idx, me_.allocator());
    }

    NodeBase* GetLastChild(NodeBase *node)
    {
        return memoria::btree::GetLastChild<NonLeafDispatcher, NodeBase>(node, me_.allocator());
    }


    NodeBase* GetParent(NodeBase *node)
    {
//        MEMORIA_TRACE(me_, "GetParent", node->id(), node->is_root());
        if (node->is_root())
        {
//            MEMORIA_TRACE(me_, "GetParent", node->id(), "Root having id of", node->id());
            return NULL;
        }
        else
        {
            NodeBase* parent = static_cast<NodeBase*>(me_.allocator().GetPage(node->parent_id()));
            if (parent == NULL) {
//                MEMORIA_TRACE(me_, "GetParent: parent is null but id is", node->parent_id());
            }
            else {
//                MEMORIA_TRACE(me_, "GetParent: parent is", parent->id());
            }
            return parent;
        }
    }


    template <typename Max>
    class GetCapacityFn {
        Int cap_;
        Max max_node_capacity_;
    public:
        GetCapacityFn(Max max_node_capacity): max_node_capacity_(max_node_capacity) {}

        template <typename T>
        void operator()(T *node) {
            if (max_node_capacity_ == -1)
            {
                cap_ = (node->map().max_size() - node->map().size());
            }
            else
            {
                Int capacity = max_node_capacity_ - node->map().size();
                cap_ = capacity > 0 ? capacity : 0;
            }
        }

        Int cap() const {
            return cap_;
        }
    };

    Int GetCapacity(NodeBase *node)
    {
        GetCapacityFn<Int> fn(me_.max_node_capacity());
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
        GetMaxCapacityFn<Int> fn(me_.max_node_capacity());
        NodeDispatcher::Dispatch(node, fn);
        return fn.cap();
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

    static void SetRootMetadata(NodeBase *node, const Metadata& metadata)
    {
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

    void SumKeys(NodeBase *node, Int from, Int count, Key* keys) {
        SumKeysFn fn(from, count, keys);
        NodeDispatcher::Dispatch(node, fn);
    }

    struct AddKeysFn {
        Int idx_;
        Key *keys_;

        AddKeysFn(Int idx, Key* keys): idx_(idx), keys_(keys) {}

        template <typename Node>
        void operator()(Node *node) {
            for (Int c = 0; c < Indexes; c++) {
                node->map().key(c, idx_) += keys_[c];
            }
            node->map().Reindex();
        }
    };

    void AddKeys(NodeBase *node, int idx, Key* keys, bool deep = true) {
        MEMORIA_TRACE(me_, "add_keys: add values of", keys[0], "to", node->id(), "at", idx);
        AddKeysFn fn(idx, keys);
        NodeDispatcher::Dispatch(node, fn);

        if (deep && !node->is_leaf())
        {
            AddKeys(me_.GetChild(node, idx), 0, keys);
        }
    }





    NodeBase* GetNode(ID &id)
    {
        return static_cast<NodeBase*>(me_.allocator().GetPage(id));
    }


    static Int GetChildrenCount(NodeBase *node)
    {
        return memoria::btree::GetChildrenCount<NodeDispatcher>(node);
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

    NodeBase* GetRoot() const
    {
        return static_cast<NodeBase *>(me_.allocator().GetPage(me_.root()));
    }

    void SetKeys(NodeBase *node, Int idx, const Key *keys) {
    	MEMORIA_TRACE(me_, node->id(), idx, keys[0]);
        memoria::btree::SetKeys<NodeDispatcher>(node, idx, keys);
    }

    void SetChildrenCount(NodeBase *node, Int count) {
        memoria::btree::SetChildrenCount<NodeDispatcher>(node, count);
    }

    void AddChildrenCount(NodeBase *node, Int count) {
    	memoria::btree::AddChildrenCount<NodeDispatcher>(node, count);
    }

    void SetINodeData(NodeBase *node, Int idx, const ID *id) {
    	MEMORIA_TRACE(me_, node->id(), idx, *id);
        memoria::btree::SetData<NonLeafDispatcher>(node, idx, id);
    }

    void Reindex(NodeBase *node) {
        memoria::btree::Reindex<NodeDispatcher>(node);
    }

    void SetLeafDataAndReindex(NodeBase *node, Int idx, const Key *keys, const Value &val) {
    	MEMORIA_TRACE(me_, node->id(), idx, keys[0], val);
        memoria::btree::SetKeyDataAndReindex<LeafDispatcher>(node, idx, keys, &val);
    }
    
    static Value GetLeafData(NodeBase *node, Int idx) {
        return *memoria::btree::GetData<LeafDispatcher, Value>(node, idx);
    }

    void SetLeafData(NodeBase *node, Int idx, const Value &val) {
        MEMORIA_TRACE(me_, node->id(), idx, val);
        memoria::btree::SetData<LeafDispatcher>(node, idx, &val);
    }

MEMORIA_CONTAINER_PART_END

}


#endif	/* _MEMORIA_PROTOTYPES_BTREE_MODEL_TOOLS_HPP */
