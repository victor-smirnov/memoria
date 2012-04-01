
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_PAGES_TOOLS_HPP

#include <iostream>
#include <memoria/core/types/typemap.hpp>
#include <memoria/core/exceptions/npe.hpp>
#include <memoria/core/types/type2type.hpp>



namespace memoria    {
namespace btree      {

using namespace memoria::vapi;

template <typename Int>
class MoveElementsFn {
    Int from_, count_;
    bool increase_children_count_;
    Int total_children_count_;
public:
    MoveElementsFn(Int from, Int count, bool increase_children_count = true):
    	from_(from), count_(count), increase_children_count_(increase_children_count) {}

    template <typename Node>
    void operator()(Node *node)
    {
        node->map().MoveData(from_ + count_, from_, node->children_count() - from_);

        for (Int c = from_; c < from_ + count_; c++)
        {
            node->map().key(c)  = 0;
            node->map().data(c) = 0;
        }

        if (increase_children_count_)
        {
        	node->inc_size(count_);
        	total_children_count_ = node->children_count();
        }
        else
        {
        	total_children_count_ = node->children_count() + count_;
        }
    }

    Int total_children_count() const {
    	return total_children_count_;
    }
};


template <typename Int>
class CopyElementsFn {
    Int count_;
    Int from_;
    Int shift_;
public:

    CopyElementsFn(Int from, Int shift): from_(from), shift_(shift){}

    template <typename Node1,typename Node2>
    void operator()(Node1 *one, Node2 *two)
    {
        count_ = one->children_count() - from_;

        Int Indexes = Node1::INDEXES;

        if (two->children_count() > 0)
        {
            two->map().MoveData(count_ + shift_, 0, two->children_count());
        }

        one->map().CopyData(from_, count_, two->map(), shift_);

        for (Int c = from_; c < from_ + count_; c++)
        {
            for (Int d = 0; d < Indexes; d++)
            {
                one->map().key(d, c) = 0;
            }
            one->map().data(c) = 0;
        }

        one->inc_size(-count_);
        two->inc_size(count_ + shift_);

        for (Int c = 0; c < shift_; c++)
        {
            for (Int d = 0; d < Indexes; d++)
            {
                two->map().key(d, c) = 0;
            }
            two->map().data(c) = 0;
        }

        one->Reindex();
        two->Reindex();
    }

    Int count() {
        return count_;
    }
};

template <typename Dispatcher, typename Node>
Int CopyElements(Node *one, Node *two, Int from, Int shift) {
    CopyElementsFn<Int> fn(from, shift);
    Dispatcher::Dispatch(one, two, fn);
    return fn.count();
}




template <typename Int>
class RemoveElementsFn {

    Int from_;
    Int count_;
    bool reindex_;

public:

    RemoveElementsFn(Int from, Int count, bool reindex):
            from_(from), count_(count), reindex_(reindex) {}

    template <typename Node>
    void operator()(Node *node)
    {
        if (from_ + count_ < node->children_count())
        {
            node->map().MoveData(from_, from_ + count_, node->children_count() - (from_ + count_));
        }

        for (Int c = node->children_count() - count_; c < node->children_count(); c++)
        {
            for (Int d = 0; d < Node::Map::INDEXES; d++)
            {
                node->map().key(d, c) = 0;
            }
            node->map().data(c) = 0;
        }

        node->inc_size(-count_);

        if (reindex_)
        {
            node->map().Reindex();
        }
    }
};

template <typename Dispatcher, typename Node, typename Int>
void RemoveElements(Node *node, Int from, Int count, bool reindex)
{
    RemoveElementsFn<Int> fn(from, count, reindex);
    Dispatcher::Dispatch(node, fn);
}



template <typename NodePage1, typename NodePage2, typename Allocator>
static NodePage2 *Node2Node(NodePage1 *src, bool root)
{
    long buffer[Allocator::PAGE_SIZE/sizeof(long)];

    for (UInt c = 0; c < sizeof(buffer)/sizeof(long); c++) buffer[c] = 0;

    //FIXME type pruning
    NodePage2 *tgt = T2T<NodePage2*>(&buffer);

    tgt->CopyFrom(src);
    tgt->set_root(root);


    tgt->page_type_hash()   = NodePage2::hash();
//    tgt->model_hash()       = src->model_hash();

    // FIXME: why we don't set tgt->map.size() here?
    // check it!!!

    for (Int c = 0; c < src->children_count(); c++)
    {
        for (Int d = 0; d < NodePage1::INDEXES; d++)
        {
            tgt->map().key(d, c) = src->map().key(d, c);
        }
        tgt->map().data(c) = src->map().data(c);
    }

    for (Int c = tgt->children_count(); c < tgt->map().max_size(); c++)
    {
        for (Int d = 0; d < NodePage2::INDEXES; d++)
        {
            tgt->map().key(d, c) = 0;
        }
        tgt->map().data(c) = 0;
    }

    tgt->map().Reindex();

    long* psrc = (long*)src;
    for (Int c = 0; c < Allocator::PAGE_SIZE/(long)sizeof(long); c++)
    {
        psrc[c] = buffer[c];
    }

    return (NodePage2*)psrc;
}


struct CopyRootMetadataFn {
    template <typename Tgt, typename Src>
    void operator()(Src *src, Tgt *tgt) {
        tgt->metadata() = src->metadata();
    }
};

template <typename Dispatcher, typename Node>
void CopyRootMetadata(Node *src, Node *tgt)
{
    CopyRootMetadataFn fn;
    Dispatcher::DoubleDispatch(src, tgt, fn);
}


template <typename TypeMap, typename Base, typename Allocator>
class Node2RootFn {
    Base *node_;

public:
    Node2RootFn() {}

    template <typename T>
    void operator()(T *node) {
        typedef typename memoria::Type2TypeMap<T, TypeMap>::Result RootType;
        node_ = Node2Node<T, RootType, Allocator>(node, true);
    }

    Base *node() {
        return node_;
    }
};

template <typename Dispatcher, typename TypeMap, typename Allocator, typename Node>
Node *Node2Root(Node *node)
{
    Node2RootFn<TypeMap, Node, Allocator> fn;
    Dispatcher::Dispatch(node, fn);
    return fn.node();
}

template <typename TypeMap, typename Base, typename Allocator>
class Root2NodeFn {
    Base *node_;
public:
    Root2NodeFn(): node_(NULL) {}

    template <typename T>
    void operator()(T *node) {
        typedef typename memoria::Type2TypeMap<T, TypeMap>::Result Type;
        node_ = Node2Node<T, Type, Allocator>(node, false);
    }

    Base *node() {
        return node_;
    }
};

template <typename Dispatcher, typename TypeMap, typename Allocator, typename Node>
Node *Root2Node(Node *node)
{
    Root2NodeFn<TypeMap, Node, Allocator> fn;
    Dispatcher::Dispatch(node, fn);
    return fn.node();
}


template <typename Base, typename Idx, typename Mgr>
class GetChildFn {
    Base node_;
    Idx idx_;
    Mgr &allocator_;
    Int flags_;
    
public:
    GetChildFn(Idx idx, Mgr &allocator, Int flags): node_(),
            idx_(idx), allocator_(allocator), flags_(flags) {}

    template <typename T>
    void operator()(T *node) {
        node_ = allocator_.GetPage(node->map().data(idx_), flags_);
    }

    Base& node() {
        return node_;
    }
};

template <typename Dispatcher, typename Base, typename Node, typename I, typename Allocator>
Base GetChild(Node node, I idx, Allocator &allocator, Int flags)
{
    GetChildFn<Base, I, Allocator> fn(idx, allocator, flags);
    Dispatcher::DispatchConst(node, fn);
    if (fn.node() != NULL)
    {
        return fn.node();
    }
    else {
        throw NullPointerException(MEMORIA_SOURCE, "Child must not be NULL");
    }
}

template <typename Base, typename Mgr>
class GetLastChildFn {
    Base    node_;
    Mgr&    allocator_;
    Int 	flags_;
    
public:
    GetLastChildFn(Mgr &allocator, Int flags): node_(), allocator_(allocator), flags_(flags) {}

    template <typename T>
    void operator()(T *node) {
        node_ = allocator_.GetPage(node->map().data(node->children_count() - 1), flags_);
    }

    Base& node() {
        return node_;
    }
};

template <typename Dispatcher, typename Base, typename Node, typename Allocator>
Base GetLastChild(Node *node, Allocator &allocator, Int flags)
{
    GetLastChildFn<Base, Allocator> fn(allocator, flags);
    Dispatcher::DispatchConst(node, fn);
    return fn.node();
}


template <typename Int>
class SetChildrenCountFn {
    Int count_;
public:
    SetChildrenCountFn(Int count): count_(count) {}

    template <typename T>
    void operator()(T *node)
    {
        node->set_children_count(count_);
    }
};



template <typename Dispatcher, typename Node, typename Int>
void SetChildrenCount(Node *node, Int count)
{
    SetChildrenCountFn<Int> fn(count);
    Dispatcher::Dispatch(node, fn);
}

template <typename Int>
class AddChildrenCountFn {
    Int count_;
public:
    AddChildrenCountFn(Int count): count_(count) {}

    template <typename T>
    void operator()(T *node)
    {
        node->inc_size(count_);
    }
};


template <typename Dispatcher, typename Node, typename Int>
void AddChildrenCount(Node *node, Int count)
{
    AddChildrenCountFn<Int> fn(count);
    Dispatcher::Dispatch(node, fn);
}


template <typename I, typename Data>
class SetDataItemFn {
    I i_;
    Data *data_;
public:
    SetDataItemFn(I i, Data *data): i_(i), data_(data) {}

    template <typename T>
    void operator()(T *node) {
        node->map().data(i_) = *data_;
    }
};

template <typename Dispatcher, typename Node, typename I, typename Data>
void SetData(Node *node, I idx, Data *data) {
    SetDataItemFn<I, Data> fn(idx, data);
    Dispatcher::Dispatch(node, fn);
}


template <typename Data>
class GetValueItemFn {
    Int i_;
    const Data *data_;
public:
    GetValueItemFn(Int i): i_(i), data_(NULL) {}

    template <typename T>
    void operator()(T *node) {
        data_ = &node->map().data(i_);
    }

    const Data* data() const {
        return data_;
    }
};

template <typename Dispatcher, typename Data, typename Node>
const Data* GetValue(Node *node, Int idx) {
    GetValueItemFn<Data> fn(idx);
    Dispatcher::DispatchConst(node, fn);
    return fn.data();
}

struct ReindexFn {
    template <typename T>
    void operator()(T *node) {
        node->map().Reindex();
    }
};

template <typename Dispatcher, typename Node>
void Reindex(Node *node)
{
    ReindexFn fn;
    Dispatcher::Dispatch(node, fn);
}

template <typename I, typename Keys>
class SetKeysFn {
    I       i_;
    Keys*   keys_;
public:
    SetKeysFn(I i, Keys *keys): i_(i), keys_(keys) {}

    template <typename T>
    void operator()(T *node) {
        for (Int c = 0; c < T::Map::INDEXES; c++) {
            node->map().key(c, i_) = keys_[c];
        }
    }
};

template <typename Dispatcher, typename Node, typename I, typename Keys>
void SetKeys(Node *node, I idx, Keys *keys) {
    SetKeysFn<I, Keys> fn(idx, keys);
    Dispatcher::Dispatch(node, fn);
}


template <typename I, typename Key>
class SetKeyFn {
    I d_;
    I i_;
    Key key_;
public:
    SetKeyFn(I d, I i, Key key): d_(d), i_(i), key_(key) {}

    template <typename T>
    void operator()(T *node) {
        node->map().key(d_, i_) = key_;
    }
};

template <typename Dispatcher, typename Node, typename I, typename Key>
void SetKey(Node *node, I d, I idx, Key key) {
    SetKeyFn<I, Key> fn(d, idx, key);
    Dispatcher::Dispatch(node, fn);
}


template <typename Key, typename I>
class GetKeyFn {
    Key     key_;
    I       i_;
    I       idx_;
public:
    GetKeyFn(I i, I idx): key_(0), i_(i), idx_(idx) {}

    template <typename T>
    void operator()(T *node) {
        key_ = node->map().key(i_, idx_);
    }

    Key key() const {
        return key_;
    }
};

template <typename Dispatcher, typename Key, typename Node, typename Idx>
Key GetKey(const Node *node, Idx i, Idx idx)
{
    GetKeyFn<Key, Idx> fn(i, idx);
    Dispatcher::DispatchConst(node, fn);
    return fn.key();
}

template<typename Key, typename I>
class GetMaxKeyFn {
    Key key_;
    I i_;
public:
    GetMaxKeyFn(I i): key_(0), i_(i) {}

    template <typename T>
    void operator()(T *node) {
        key_ = node->map().max_key(i_);
    }

    Key key() {
        return key_;
    }
};


template <typename Dispatcher, typename Key, typename Node, typename I>
Key GetMaxKey(const Node *node, I i)
{
    GetMaxKeyFn<Key, I> fn(i);
    Dispatcher::DispatchConst(node, fn);
    return fn.key();
}

template<typename Key, Int Indexes>
class GetMaxKeysFn {
    Key* keys_;
public:
    GetMaxKeysFn(Key* keys): keys_(keys) {}

    template <typename T>
    void operator()(T *node) {
        for (Int c = 0; c < Indexes; c++) {
            keys_[c] = node->map().max_key(c);
        }
    }
};

template <typename Dispatcher, Int Indexes, typename Key, typename Node>
void GetMaxKeys(const Node *node, Key* keys)
{
    GetMaxKeysFn<Key, Indexes> fn(keys);
    Dispatcher::DispatchConst(node, fn);
}

template<typename Key, Int Indexes>
class GetKeysFn {
    Key* keys_;
    Int idx_;
public:
    GetKeysFn(Int idx, Key* keys): keys_(keys), idx_(idx) {}

    template <typename T>
    void operator()(T *node) {
        for (Int c = 0; c < Indexes; c++) {
            keys_[c] = node->map().key(c, idx_);
        }
    }
};

template <typename Dispatcher, Int Indexes, typename Key, typename Node>
const void GetKeys(const Node *node, Int idx, Key* keys)
{
    GetKeysFn<Key, Indexes> fn(idx, keys);
    Dispatcher::DispatchConst(node, fn);
}

} //btree
} //memoria




#endif
