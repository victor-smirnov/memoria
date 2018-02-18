
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/config.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <memoria/v1/core/tools/hash.hpp>

#include <iostream>

namespace memoria {
namespace v1 {

using namespace v1::bt;


MEMORIA_V1_BT_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTIteratorBase)
public:
    using Types     = typename Base::Container::Types;
    using NodeBase  = typename Types::NodeBase;
    using NodeBaseG = typename Types::NodeBaseG;
    using Allocator = typename Base::Container::Allocator;

    using BranchNodeEntry = typename Types::BranchNodeEntry;

    using CtrSizeT  = typename Types::CtrSizeT;
    using CtrSizesT = typename Types::CtrSizesT;

    using Iterator  = typename Base::Container::Iterator;

    using IteratorCache = typename Types::template IteratorCacheFactory<
            MyType,
            typename Base::Container
    >::Type;


private:

    NodeBaseG           leaf_;

    int32_t                 idx_;
    int32_t                 stream_;

    bool                found_;

    IteratorCache       cache_;

public:
    BTIteratorBase():
        Base(), idx_(0), stream_(0), found_(false)
    {
//        cache_.init(&self());
    }

    BTIteratorBase(ThisType&& other):
        Base(std::move(other)),
        leaf_(other.leaf_),
        idx_(other.idx_),
        stream_(other.stream_),
        cache_(std::move(other.cache_))
    {
        cache_.init(&self());
    }

    BTIteratorBase(const ThisType& other):
        Base(other),
        leaf_(other.leaf_),
        idx_(other.idx_),
        stream_(other.stream_),
        cache_(other.cache_)
    {
        //cache_.init(me());
    }

    void assign(ThisType&& other)
    {
        leaf_       = other.leaf_;
        idx_        = other.idx_;
        stream_     = other.stream_;
        found_      = other.found_;

        cache_      = other.cache_;

        Base::assign(std::move(other));
    }

    void assign(const ThisType& other)
    {
        leaf_       = other.leaf_;
        idx_        = other.idx_;
        found_      = other.found_;
        stream_     = other.stream_;

        cache_      = other.cache_;

        Base::assign(other);
    }

    auto clone() const
    {
        return self().ctr().clone_iterator(self());
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


    int32_t& stream() {
        return stream_;
    }

    const int32_t& stream() const {
        return stream_;
    }

    int32_t &key_idx()
    {
        return idx_;
    }

    const int32_t key_idx() const
    {
        return idx_;
    }

    int32_t &idx()
    {
        return idx_;
    }

    const int32_t idx() const
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

    bool is_end() const
    {
        auto& self = this->self();
        return leaf().isSet() ? idx() >= self.leaf_size() : true;
    }

    bool isEnd(int32_t idx) const
    {
        auto& self = this->self();

        return leaf().isSet() ? idx >= self.leaf_size() : true;
    }

    bool isContent() const
    {
        auto& self = this->self();
        return !(self.isBegin() || self.isEnd());
    }

    bool isContent(int32_t idx) const
    {
        auto& self = this->self();

        bool is_set = self.leaf().isSet();

        auto leaf_size = self.leaf_size();

        return is_set && idx >= 0 && idx < leaf_size;
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

    int64_t keyNum() const
    {
        return cache_.key_num();
    }

    int64_t& keyNum()
    {
        return cache_.key_num();
    }


    bool has_same_leaf(const Iterator& other) const
    {
        return self().leaf()->id() == other.leaf()->id();
    }


    void dump(std::ostream& out = cout, const char* header = nullptr) const
    {
        auto& self = this->self();

        out<<(header != NULL ? header : self.getDumpHeader())<<std::endl;

        self.dumpKeys(out);
        self.dumpCache(out);

        self.dumpBeforePages(out);
        self.dumpPages(out);
    }

    U8String getDumpHeader() const
    {
        return U8String(self().ctr().typeName()) + " Iterator State";
    }

    void dumpPath(std::ostream& out = std::cout, const char* header = nullptr) const
    {
        auto& self  = this->self();
        out<<(header != NULL ? header : self.getDumpHeader())<<std::endl;
        dumpCache(out);
        dumpKeys(out);
        self.ctr().dumpPath(self.leaf(), out);
        out<<"======================================================================"<<std::endl;
    }

    void dumpCache(std::ostream& out = std::cout) const
    {
        auto& self  = this->self();
        out<<self.cache()<<std::endl;
    }

    void dumpHeader(std::ostream& out = std::cout) const
    {
        self().dumpCache(out);
        self().dumpKeys(out);
        if (self().leaf().isSet()) {
            cout<<"Node ID: "<<self().leaf()->id()<<endl;
        }
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

    void prepare() {}

    void init() {}

    void refresh() {}

public:
    template <typename Walker>
    void finish_walking(int32_t idx, const Walker& w, WalkCmd cmd) {}



MEMORIA_BT_ITERATOR_BASE_CLASS_END

}}
