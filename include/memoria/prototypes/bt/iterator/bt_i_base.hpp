
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_BASE_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_BASE_H

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/core/tools/hash.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::bt;


MEMORIA_BT_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTIteratorBase)
public:
    typedef typename Base::Container::Types                                             Types;
    typedef typename Base::Container::NodeBase                                          NodeBase;
    typedef typename Base::Container::NodeBaseG                                         NodeBaseG;
    typedef typename Base::Container::Allocator                                         Allocator;
    typedef typename Base::Container::Accumulator                                       Accumulator;

    typedef typename Types::template IteratorCacheFactory<
            MyType,
            typename Base::Container
    >::Type                                                                             IteratorCache;


private:

    NodeBaseG           leaf_;

    Int                 idx_;
    Int                 stream_;

    bool                found_;

    IteratorCache       cache_;

public:
    BTIteratorBase():
        Base(), idx_(0), stream_(0), found_(false)
    {
        cache_.init(me());
    }

    BTIteratorBase(ThisType&& other):
        Base(std::move(other)),
        leaf_(other.leaf_),
        idx_(other.idx_),
        stream_(other.stream_),
        cache_(std::move(other.cache_))
    {
        cache_.init(me());
    }

    BTIteratorBase(const ThisType& other):
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
        leaf_       = other.leaf_;
        idx_        = other.idx_;
        stream_     = other.stream_;
        found_      = other.found_;

        cache_      = other.cache_;

        cache_.init(me());

        Base::assign(std::move(other));
    }

    void assign(const ThisType& other)
    {
        leaf_       = other.leaf_;
        idx_        = other.idx_;
        found_      = other.found_;
        stream_     = other.stream_;

        cache_      = other.cache_;

        cache_.init(me());

        Base::assign(other);
    }

    const bool& found() const {
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

    bool isContent() const
    {
    	auto& self = this->self();
    	return !(self.isBegin() || self.isEnd());
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


    void dump(std::ostream& out = cout, const char* header = nullptr) const
    {
        auto& self = this->self();

        out<<(header != NULL ? header : me()->getDumpHeader())<<std::endl;

        self.dumpKeys(out);

        self.dumpBeforePages(out);
        self.dumpPages(out);
    }

    String getDumpHeader() const
    {
        return String(self().ctr().typeName()) + " Iterator State";
    }

    void dumpPath(std::ostream& out = std::cout, const char* header = nullptr) const
    {
        auto& self  = this->self();
        out<<(header != NULL ? header : me()->getDumpHeader())<<std::endl;
        dumpKeys(out);
        self.ctr().dumpPath(self.leaf(), out);
        out<<"======================================================================"<<std::endl;
    }

    void dumpKeys(std::ostream& out) const
    {
        auto& self = this->self();

        out<<"Stream:  "<<self.stream()<<std::endl;
        out<<"Idx:  "<<self.idx()<<std::endl;
    }

    void dumpBeforePath(std::ostream& out) const {}
    void dumpBeforePages(std::ostream& out) const {}

    void dumpPages(std::ostream& out) const
    {
        auto& self = this->self();

        self.ctr().dump(self.leaf(), out);
    }

    void init() {}

MEMORIA_BT_ITERATOR_BASE_CLASS_END

} //memoria



#endif
