
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

    bool                found_;

    IteratorCache       cache_;

public:
    BalTreeIteratorBase(): Base(), path_(), key_idx_(0)
    {
        cache_.init(me());
    }

    BalTreeIteratorBase(ThisType&& other):
        Base(std::move(other)), path_(std::move(other.path_)), key_idx_(other.key_idx_), cache_(std::move(other.cache_))
    {
        cache_.init(me());
    }

    BalTreeIteratorBase(const ThisType& other): Base(other), path_(other.path_), key_idx_(other.key_idx_), cache_(other.cache_)
    {
        cache_.init(me());
    }

    void assign(ThisType&& other)
    {
        path_       = other.path_;
        key_idx_    = other.key_idx_;
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

        cache_      = other.cache_;

        cache_.init(me());

        Base::assign(other);
    }

//    const BigInt prefix(Int num = 0) const
//    {
//        return me()->cache().prefix(num);
//    }
//
//    const Accumulator prefixes() const
//    {
//        return me()->cache().prefixes();
//    }
//
//    void setupPrefix(Key prefix, Int key_num)
//    {
//        me()->cache().setup(prefix, key_num);
//    }
//
//
//    Accumulator get_prefixes() const
//    {
//        Accumulator accum;
//
//        const TreePath& path0 = me()->path();
//        Int             idx   = me()->key_idx();
//
//        for (Int c = 0; c < path0.getSize(); c++)
//        {
//            me()->model().sumKeys(path0[c].node(), 0, idx, accum);
//            idx = path0[c].parent_idx();
//        }
//
//        return accum;
//    }
//
//    Key get_prefix(Int block_num) const
//    {
//        Key accum = 0;
//
//        const TreePath& path0 = me()->path();
//        Int             idx   = me()->key_idx();
//
//        for (Int c = 0; c < path0.getSize(); c++)
//        {
//            me()->model().sumKeys(path0[c].node(), block_num, 0, idx, accum);
//            idx = path0[c].parent_idx();
//        }
//
//        return accum;
//    }


    bool IsFound() const {
        return found_;
    }

    void setFound(bool found)
    {
        found_ = found;
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

    Int &key_idx()
    {
        return key_idx_;
    }

    const Int key_idx() const
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
        return page().isSet() ? key_idx() >= self().model().getNodeSize(page(), 0) : true;
    }

    bool isNotEnd() const
    {
        return !isEnd();
    }

    bool isEmpty() const
    {
        return page().isEmpty() || self().model().getNodeSize(page(), 0) == 0;
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
        out<<(header != NULL ? header : me()->getDumpHeader())<<endl;

        me()->dumpKeys(out);

        me()->dumpBeforePath(out);
        me()->dumpPath(out);

        me()->dumpBeforePages(out);
        me()->dumpPages(out);
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
        out<<"KeyIdx:  "<<me()->key_idx()<<endl;
    }

    void dumpBeforePath(ostream& out){}
    void dumpBeforePages(ostream& out){}

    void dumpPages(ostream& out)
    {
        me()->model().dump(me()->path().leaf(), out);
    }

MEMORIA_BALTREE_ITERATOR_BASE_CLASS_END

} //memoria



#endif
