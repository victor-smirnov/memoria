
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/hash.hpp>

#include <iostream>

namespace memoria {
namespace v1 {

MEMORIA_V1_BT_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTIteratorBase)
public:
    using Types     = typename Base::Container::Types;
    using NodeBaseG = typename Types::NodeBaseG;
    using Allocator = typename Base::Container::Allocator;

    using BranchNodeEntry = typename Types::BranchNodeEntry;

    using CtrSizeT  = typename Types::CtrSizeT;
    using CtrSizesT = typename Types::CtrSizesT;

    using Iterator  = typename Base::Container::Iterator;

    using IteratorCache = typename Types::template IteratorCacheFactory<
            MyType,
            typename Base::Container
    >;

    using CtrT = Ctr<Types>;

    using IOVectorViewT = typename Types::LeafNode::template SparseObject<CtrT>::IOVectorViewT;

private:

    NodeBaseG           leaf_;

    IOVectorViewT       iovector_view_;

    int32_t             idx_;
    int32_t             stream_;

    IteratorCache       cache_;

public:
    BTIteratorBase():
        Base(), idx_(0), stream_(0)
    {
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
    }


    void assign(ThisType&& other)
    {
        leaf_       = other.leaf_;
        idx_        = other.idx_;
        stream_     = other.stream_;
        cache_      = other.cache_;

        refresh_iovector_view();

        Base::assign(std::move(other));
    }

    void assign(const ThisType& other)
    {
        leaf_       = other.leaf_;
        idx_        = other.idx_;
        stream_     = other.stream_;

        cache_      = other.cache_;

        refresh_iovector_view();

        Base::assign(other);
    }

    auto iter_clone() const
    {
        return self().ctr().clone_iterator(self());
    }

    bool iter_equals(const ThisType& other) const
    {
        return iter_leaf().node() == other.iter_leaf().node() && idx_ == other.idx_ && Base::iter_equals(other);
    }

    bool iter_not_equals(const ThisType& other) const
    {
        return iter_leaf().node() != other.iter_leaf().node() || idx_ != other.idx_ || Base::iter_not_equals(other);
    }


    int32_t& iter_stream() {
        return stream_;
    }


    int32_t iter_stream() const {
        return stream_;
    }


    int32_t &iter_local_pos()
    {
        return idx_;
    }

    int32_t iter_local_pos() const
    {
        return idx_;
    }

    class NodeAccessor {
        NodeBaseG& node_;

        MyType& iter_;

    public:
        NodeAccessor(NodeBaseG& node, MyType& iter): node_(node), iter_(iter) {}

        operator NodeBaseG&() {return node_;}

        void assign(NodeBaseG node)
        {
            node_ = node;
            iter_.refresh_iovector_view();
        }

        NodeBaseG& node() {
            return node_;
        }

        auto operator->() {
            return node_.operator->();
        }
    };


    class ConstNodeAccessor {
        const NodeBaseG& node_;
    public:
        ConstNodeAccessor(const NodeBaseG& node): node_(node) {}
        operator const NodeBaseG&() const {return node_;}

        const auto operator->() const {
            return node_.operator->();
        }

        const NodeBaseG& node() const {
            return node_;
        }
    };

    NodeAccessor iter_leaf()
    {
        return NodeAccessor(leaf_, self());
    }

    ConstNodeAccessor iter_leaf() const
    {
        return ConstNodeAccessor(leaf_);
    }


    const io::IOVector& iovector_view() const {
        return iovector_view_;
    }

    MEMORIA_V1_DECLARE_NODE_FN(RefreshIOVectorViewFn, configure_iovector_view);
    void refresh_iovector_view()
    {
        self().ctr().leaf_dispatcher().dispatch(leaf_, RefreshIOVectorViewFn(), *&iovector_view_);
    }


    IteratorCache& iter_cache() {
        return cache_;
    }

    const IteratorCache& iter_cache() const {
        return cache_;
    }


    bool isBegin() const
    {
        return iter_local_pos() < 0 || isEmpty();
    }

    bool isEnd() const
    {
        auto& self = this->self();

        return iter_leaf().node().isSet() ? iter_local_pos() >= self.iter_leaf_size() : true;
    }

    bool is_end() const
    {
        auto& self = this->self();
        return iter_leaf().node().isSet() ? iter_local_pos() >= self.iter_leaf_size() : true;
    }

    bool isEnd(int32_t idx) const
    {
        auto& self = this->self();
        return iter_leaf().node().isSet() ? idx >= self.iter_leaf_size() : true;
    }

    bool isContent() const
    {
        auto& self = this->self();
        return !(self.isBegin() || self.isEnd());
    }

    bool isContent(int32_t idx) const
    {
        auto& self = this->self();

        bool is_set = self.iter_leaf().node().isSet();

        auto iter_leaf_size = self.iter_leaf_size();

        return is_set && idx >= 0 && idx < iter_leaf_size;
    }

    bool isNotEnd() const
    {
        return !isEnd();
    }

    bool isEmpty() const
    {
        auto& self = this->self();
        return (iter_leaf().node().isEmpty()) || (self.iter_leaf_size() == 0);
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
        return self().iter_leaf()->id() == other.iter_leaf()->id();
    }


    void dump(std::ostream& out = std::cout, const char* header = nullptr) const
    {
        auto& self = this->self();

        out << (header != NULL ? header : self.getDumpHeader()) << std::endl;

        self.iter_dump_keys(out);
        self.dumpCache(out);

        self.dumpBeforePages(out);
        self.dumpPages(out);
    }

    U8String getDumpHeader() const
    {
        return U8String(self().ctr().type_name_str()) + " Iterator State";
    }

    void dumpPath(std::ostream& out = std::cout, const char* header = nullptr) const
    {
        auto& self  = this->self();
        out << (header != NULL ? header : self.getDumpHeader()) << std::endl;
        dumpCache(out);
        iter_dump_keys(out);
        self.ctr().ctr_dump_path(self.iter_leaf(), out);
        out << "======================================================================" << std::endl;
    }

    void dumpCache(std::ostream& out = std::cout) const
    {
        auto& self  = this->self();
        out << self.iter_cache() << std::endl;
    }

    void dumpHeader(std::ostream& out = std::cout) const
    {
        self().dumpCache(out);
        self().iter_dump_keys(out);
        if (self().iter_leaf().node().isSet()) {
            std::cout << "Node ID: " << self().iter_leaf()->id() << std::endl;
        }
    }

    void iter_dump_keys(std::ostream& out) const
    {
        auto& self = this->self();

        out << "Stream:  " << self.iter_stream() << std::endl;
        out << "Idx:  " << self.iter_local_pos() << std::endl;
    }

    void dumpBeforePath(std::ostream& out) const {}
    void dumpBeforePages(std::ostream& out) const {}

    void dumpPages(std::ostream& out) const
    {
        auto& self = this->self();

        self.ctr().ctr_dump_node(self.iter_leaf(), out);
    }

    void iter_prepare() {}

    void iter_init() {}

    void iter_refresh() {}

public:
    template <typename Walker>
    void finish_walking(int32_t idx, const Walker& w, WalkCmd cmd) {}



MEMORIA_BT_ITERATOR_BASE_CLASS_END

}}
