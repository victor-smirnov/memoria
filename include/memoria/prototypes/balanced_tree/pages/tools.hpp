
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_TOOLS_HPP

#include <memoria/core/types/typemap.hpp>
#include <memoria/core/exceptions/npe.hpp>
#include <memoria/core/types/type2type.hpp>

#include <iostream>
#include <string.h>

namespace memoria    	{
namespace balanced_tree {

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
        node->map().moveData(from_ + count_, from_, node->children_count() - from_);

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
            two->map().moveData(count_ + shift_, 0, two->children_count());
        }

        one->map().copyData(from_, count_, two->map(), shift_);

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

        one->reindex();
        two->reindex();
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
            node->map().moveData(from_, from_ + count_, node->children_count() - (from_ + count_));
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
            node->map().reindex();
        }
    }
};

template <typename Dispatcher, typename Node, typename Int>
void RemoveElements(Node *node, Int from, Int count, bool reindex)
{
    RemoveElementsFn<Int> fn(from, count, reindex);
    Dispatcher::Dispatch(node, fn);
}


struct CopyRootMetadataFn {
    template <typename Tgt, typename Src>
    void operator()(Src *src, Tgt *tgt) {
        tgt->root_metadata() = src->root_metadata();
    }
};

template <typename Dispatcher, typename Node>
void copyRootMetadata(Node *src, Node *tgt)
{
    CopyRootMetadataFn fn;
    Dispatcher::DoubleDispatch(src, tgt, fn);
}


template <typename TypeMap, typename Allocator, typename Metadata>
class Node2RootFn {
    const Metadata& metadata_;

public:
    Node2RootFn(const Metadata& metadata): metadata_(metadata) {}

    template <typename T>
    void operator()(T *src)
    {
        typedef typename memoria::Type2TypeMap<T, TypeMap>::Result RootType;

        RootType* tgt = T2T<RootType*>(malloc(src->page_size()));
        memset(tgt, 0, src->page_size());

        tgt->map().init(src->page_size() - sizeof(RootType) + sizeof(typename RootType::Map));
        tgt->copyFrom(src);

        tgt->root_metadata() = metadata_;

        tgt->set_root(true);

        tgt->page_type_hash()   = RootType::hash();

        src->map().transferDataTo(&tgt->map());

        tgt->set_children_count(src->children_count());

        tgt->map().clearUnused();

        tgt->map().reindex();

        CopyByteBuffer(tgt, src, tgt->page_size());

        free(tgt);

    }
};



template <typename TypeMap, typename Allocator>
class Root2NodeFn {
public:
    Root2NodeFn() {}

    template <typename T>
    void operator()(T *src)
    {
        typedef typename memoria::Type2TypeMap<T, TypeMap>::Result NonRootNode;

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
};



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
        node_ = allocator_.getPage(node->map().data(idx_), flags_);
    }

    Base& node() {
        return node_;
    }
};

template <typename Dispatcher, typename Base, typename Node, typename I, typename Allocator>
Base getChild(Node node, I idx, Allocator &allocator, Int flags)
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
    Int     flags_;
    
public:
    GetLastChildFn(Mgr &allocator, Int flags): node_(), allocator_(allocator), flags_(flags) {}

    template <typename T>
    void operator()(T *node) {
        node_ = allocator_.getPage(node->map().data(node->children_count() - 1), flags_);
    }

    Base& node() {
        return node_;
    }
};

template <typename Dispatcher, typename Base, typename Node, typename Allocator>
Base getLastChild(Node *node, Allocator &allocator, Int flags)
{
    GetLastChildFn<Base, Allocator> fn(allocator, flags);
    Dispatcher::DispatchConst(node, fn);
    return fn.node();
}



//template <typename I, typename Key>
//class SetKeyFn {
//    I d_;
//    I i_;
//    Key key_;
//public:
//    SetKeyFn(I d, I i, Key key): d_(d), i_(i), key_(key) {}
//
//    template <typename T>
//    void operator()(T *node) {
//        node->map().key(d_, i_) = key_;
//    }
//};
//
//template <typename Dispatcher, typename Node, typename I, typename Key>
//void setKey(Node *node, I d, I idx, Key key) {
//    SetKeyFn<I, Key> fn(d, idx, key);
//    Dispatcher::Dispatch(node, fn);
//}



} //btree
} //memoria




#endif
