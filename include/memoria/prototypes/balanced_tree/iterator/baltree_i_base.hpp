
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_BASE_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_BASE_H

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/balanced_tree/baltree_types.hpp>
#include <memoria/prototypes/balanced_tree/baltree_macros.hpp>

#include <memoria/core/tools/hash.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::balanced_tree;


MEMORIA_BALTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BalTreeIteratorBase)
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

    TreePath            path_;
    Int                 key_idx_;
    Int 				stream_;

    bool                found_;

    IteratorCache       cache_;

public:
    BalTreeIteratorBase():
    	Base(), path_(), key_idx_(0), stream_(0), found_(false)
    {
        cache_.init(me());
    }

    BalTreeIteratorBase(ThisType&& other):
        Base(std::move(other)),
        path_(std::move(other.path_)),
        key_idx_(other.key_idx_),
        stream_(other.stream_),
        cache_(std::move(other.cache_))
    {
        cache_.init(me());
    }

    BalTreeIteratorBase(const ThisType& other):
    	Base(other),
    	path_(other.path_),
    	key_idx_(other.key_idx_),
    	stream_(other.stream_),
    	cache_(other.cache_)
    {
        cache_.init(me());
    }

    void assign(ThisType&& other)
    {
        path_       = other.path_;
        key_idx_    = other.key_idx_;
        stream_     = other.stream_;
        found_      = other.found_;

        cache_      = other.cache_;

        cache_.init(me());

        Base::assign(std::move(other));
    }

    void assign(const ThisType& other)
    {
        path_       = other.path_;
        key_idx_    = other.key_idx_;
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
        return page() == other.page() && key_idx_ == other.key_idx_ && Base::isEqual(other);
    }

    bool isNotEqual(const ThisType& other) const
    {
        return page() != other.page() || key_idx_ != other.key_idx_ || Base::isNotEqual(other);
    }

    void setNode(NodeBaseG& node, Int parent_idx)
    {
        path_[node->level()].node()         = node;
        path_[node->level()].parent_idx()   = parent_idx;
    }

    Int& stream() {
    	return stream_;
    }

    const Int& stream() const {
    	return stream_;
    }

    Int &key_idx()
    {
        return key_idx_;
    }

    const Int key_idx() const
    {
        return key_idx_;
    }

    Int &idx()
    {
    	return key_idx_;
    }

    const Int idx() const
    {
    	return key_idx_;
    }

    NodeBaseG& page()
    {
        return path_.leaf().node();
    }

    const NodeBaseG& page() const
    {
        return path_.leaf().node();
    }

    TreePathItem& leaf()
    {
        return path_.leaf();
    }

    const TreePathItem& leaf() const
    {
        return path_.leaf();
    }

    TreePath& path()
    {
        return path_;
    }

    const TreePath& path() const
    {
        return path_;
    }

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

    	return page().isSet() ? idx() >= self.leaf_size() : true;
    }

    bool isNotEnd() const
    {
        return !isEnd();
    }

    bool isEmpty() const
    {
    	auto& self = this->self();
    	return page().isEmpty() || self.leaf_size() == 0;
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


    void dump(ostream& out = cout, const char* header = NULL)
    {
    	auto& self = this->self();

        out<<(header != NULL ? header : me()->getDumpHeader())<<endl;

        self.dumpKeys(out);

        self.dumpBeforePath(out);
        self.dumpPath(out);

        self.dumpBeforePages(out);
        self.dumpPages(out);
    }

    String getDumpHeader()
    {
        return String(me()->model().typeName()) + " Iterator State";
    }

    void dumpPath(ostream& out)
    {
        out<<"Path:"<<endl;

        TreePath& path0 = me()->path();
        for (int c = me()->path().getSize() - 1; c >= 0; c--)
        {
            out<<"Node("<<c<<"): "<<IDValue(path0[c]->id())
               <<" idx="<<(c > 0 ? toString(path0[c - 1].parent_idx()) : "")<<endl;
        }
    }

    void dumpKeys(ostream& out)
    {
    	auto& self = this->self();

    	out<<"Stream:  "<<self.stream()<<endl;
        out<<"Idx:  "<<self.idx()<<endl;
    }

    void dumpBeforePath(ostream& out){}
    void dumpBeforePages(ostream& out){}

    void dumpPages(ostream& out)
    {
        me()->model().dump(me()->path().leaf(), out);
    }

    void init() {}

MEMORIA_BALTREE_ITERATOR_BASE_CLASS_END

} //memoria



#endif
