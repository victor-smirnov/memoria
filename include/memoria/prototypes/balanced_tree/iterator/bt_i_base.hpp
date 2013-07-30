
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_BASE_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_BASE_H

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/balanced_tree/bt_names.hpp>
#include <memoria/prototypes/balanced_tree/bt_macros.hpp>

#include <memoria/core/tools/hash.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::balanced_tree;


MEMORIA_bt_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BalTreeIteratorBase)
public:
    typedef typename Base::Container::Types                                             Types;
    typedef typename Base::Container::TreePath                                          TreePath;
    typedef typename Base::Container::TreePath::Element                                 TreePathItem;
    typedef typename Base::Container::NodeBase                                          NodeBase;
    typedef typename Base::Container::NodeBaseG                                         NodeBaseG;
    typedef typename Base::Container::Allocator                                         Allocator;
    typedef typename Base::Container::Accumulator                                       Accumulator;
    typedef typename Base::Container::Key                                       		Key;

    typedef typename Types::template IteratorCacheFactory<
            MyType,
            typename Base::Container
    >::Type                                                                             IteratorCache;

    static const Int Indexes                                                            = Base::Container::Indexes;

private:

    NodeBaseG 			leaf_;

//    TreePath            path_;
    Int                 idx_;
    Int 				stream_;

    bool                found_;

    IteratorCache       cache_;

public:
    BalTreeIteratorBase():
    	Base(), idx_(0), stream_(0), found_(false)
    {
        cache_.init(me());
    }

    BalTreeIteratorBase(ThisType&& other):
        Base(std::move(other)),
        leaf_(other.leaf_),
        idx_(other.idx_),
        stream_(other.stream_),
        cache_(std::move(other.cache_))
    {
        cache_.init(me());
    }

    BalTreeIteratorBase(const ThisType& other):
    	Base(other),
    	leaf_(other.leaf_),
    	idx_(other.idx_),
    	stream_(other.stream_),
    	cache_(other.cache_)
    {
        cache_.init(me());
    }

    void assign(ThisType&& other)
    {
    	leaf_		= other.leaf_;
        idx_    	= other.idx_;
        stream_     = other.stream_;
        found_      = other.found_;

        cache_      = other.cache_;

        cache_.init(me());

        Base::assign(std::move(other));
    }

    void assign(const ThisType& other)
    {
    	leaf_		= other.leaf_;
        idx_    	= other.idx_;
        found_      = other.found_;
        stream_     = other.stream_;

        cache_      = other.cache_;

        cache_.init(me());

        Base::assign(other);
    }

    bool& found() const {
        return found_;
    }

    bool& found()
    {
        return found_;
    }

    bool isEqual(const ThisType& other) const
    {
        return leaf() == other.leaf() && idx_ == other.idx_ && Base::isEqual(other);
    }

    bool isNotEqual(const ThisType& other) const
    {
        return leaf() != other.leaf() || idx_ != other.idx_ || Base::isNotEqual(other);
    }

//    void setNode(NodeBaseG& node, Int parent_idx)
//    {
//        path_[node->level()].node()         = node;
//        path_[node->level()].parent_idx()   = parent_idx;
//    }

    Int& stream() {
    	return stream_;
    }

    const Int& stream() const {
    	return stream_;
    }

    Int &key_idx()
    {
        return idx_;
    }

    const Int key_idx() const
    {
        return idx_;
    }

    Int &idx()
    {
    	return idx_;
    }

    const Int idx() const
    {
    	return idx_;
    }


    NodeBaseG& leaf()
    {
        return leaf_;
    }

    const NodeBaseG& leaf() const
    {
        return leaf_;
    }

//    TreePath& path()
//    {
//        return path_;
//    }
//
//    const TreePath& path() const
//    {
//        return path_;
//    }

    IteratorCache& cache() {
        return cache_;
    }

    const IteratorCache& cache() const {
        return cache_;
    }


    bool isBegin() const
    {
        return key_idx() < 0 || isEmpty();
    }

    bool isEnd() const
    {
        auto& self = this->self();

    	return leaf().isSet() ? idx() >= self.leaf_size() : true;
    }

    bool isNotEnd() const
    {
        return !isEnd();
    }

    bool isEmpty() const
    {
    	auto& self = this->self();
    	return leaf().isEmpty() || self.leaf_size() == 0;
    }

    bool isNotEmpty() const
    {
        return !isEmpty();
    }

    BigInt keyNum() const
    {
        return cache_.key_num();
    }

    BigInt& keyNum()
    {
        return cache_.key_num();
    }


    void dump(ostream& out = cout, const char* header = nullptr) const
    {
    	auto& self = this->self();

        out<<(header != NULL ? header : me()->getDumpHeader())<<endl;

        self.dumpKeys(out);

//        self.dumpBeforePath(out);
//        self.dumpPath(out);

        self.dumpBeforePages(out);
        self.dumpPages(out);
    }

    String getDumpHeader() const
    {
        return String(self().ctr().typeName()) + " Iterator State";
    }

    void dumpPath(std::ostream& out = std::cout, const char* header = nullptr) const
    {
    	auto& self 	= this->self();
    	out<<(header != NULL ? header : me()->getDumpHeader())<<endl;
    	dumpKeys(out);
    	self.ctr().dumpPath(self.leaf(), out);
    }

    void dumpKeys(std::ostream& out) const
    {
    	auto& self = this->self();

    	out<<"Stream:  "<<self.stream()<<endl;
        out<<"Idx:  "<<self.idx()<<endl;
    }

    void dumpBeforePath(std::ostream& out) const {}
    void dumpBeforePages(std::ostream& out) const {}

    void dumpPages(std::ostream& out) const
    {
    	auto& self = this->self();

        self.ctr().dump(self.leaf(), out);
    }

    void init() {}

MEMORIA_bt_ITERATOR_BASE_CLASS_END

} //memoria



#endif
