
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

#include <memoria/core/types.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_tree_path.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_BT_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTIteratorBase)
public:
    using Types     = typename Base::Container::Types;
    using TreeNodePtr = typename Types::TreeNodePtr;
    using TreeNodeConstPtr = typename Types::TreeNodeConstPtr;
    using Allocator = typename Base::Container::Allocator;
    using Position  = typename Types::Position;
    using NodeChain = typename Base::Container::NodeChain;

    using BranchNodeEntry = typename Types::BranchNodeEntry;

    using CtrSizeT  = typename Types::CtrSizeT;
    using CtrSizesT = typename Types::CtrSizesT;

    using Iterator      = typename Base::Container::Iterator;
    using IteratorPtr   = typename Base::Container::IteratorPtr;

    using IteratorCache = typename Types::template IteratorCacheFactory<
            MyType,
            typename Base::Container
    >;

    using CtrT = Ctr<Types>;

    using IOVectorViewT = typename Types::LeafNode::template SparseObject<CtrT>::IOVectorViewT;

    using TreePathT = TreePath<TreeNodeConstPtr>;

    static constexpr int32_t Streams = Types::Streams;

private:

    TreePathT           path_;

    IOVectorViewT       iovector_view_;

    int32_t             idx_;
    int32_t             stream_;

    IteratorCache       cache_;

public:
    BTIteratorBase() noexcept:
        Base(), idx_(0), stream_(0)
    {
    }

    BTIteratorBase(ThisType&& other) noexcept:
        Base(std::move(other)),
        path_(std::move(other.path_)),
        idx_(other.idx_),
        stream_(other.stream_),
        cache_(std::move(other.cache_))
    {
        cache_.init(&self());
    }

    BTIteratorBase(const ThisType& other) noexcept:
        Base(other),
        path_(other.path_),
        idx_(other.idx_),
        stream_(other.stream_),
        cache_(other.cache_)
    {
    }


    void assign(ThisType&& other) noexcept
    {
        path_       = std::move(other.path_);
        idx_        = other.idx_;
        stream_     = other.stream_;
        cache_      = std::move(other.cache_);

        refresh_iovector_view();

        Base::assign(std::move(other));
    }

    void assign(const ThisType& other) noexcept
    {
        path_       = other.path_;
        idx_        = other.idx_;
        stream_     = other.stream_;

        cache_      = other.cache_;

        refresh_iovector_view();

        Base::assign(other);
    }

    TreePathT& path() noexcept {
        return path_;
    }

    const TreePathT& path() const noexcept {
        return path_;
    }

    auto iter_clone() const noexcept
    {
        return self().ctr().clone_iterator(self());
    }

    bool iter_equals(const ThisType& other) const noexcept
    {
        return iter_leaf().node() == other.iter_leaf().node() && idx_ == other.idx_ && Base::iter_equals(other);
    }

    bool iter_not_equals(const ThisType& other) const noexcept
    {
        return iter_leaf().node() != other.iter_leaf().node() || idx_ != other.idx_ || Base::iter_not_equals(other);
    }


    int32_t& iter_stream() noexcept {
        return stream_;
    }


    int32_t iter_stream() const noexcept{
        return stream_;
    }


    int32_t &iter_local_pos() noexcept
    {
        return idx_;
    }

    int32_t iter_local_pos() const noexcept
    {
        return idx_;
    }

    class NodeAccessor {
        TreePathT& path_;

        MyType& iter_;

    public:
        NodeAccessor(TreePathT& path, MyType& iter) noexcept:
            path_(path), iter_(iter)
        {}

        operator TreeNodeConstPtr&() noexcept {return path_.leaf();}
        operator const TreeNodeConstPtr&() const noexcept {return path_.leaf();}

        void assign(const TreeNodeConstPtr& node) noexcept
        {
            path_.leaf() = node;
            iter_.refresh_iovector_view();
        }

        TreeNodeConstPtr& node() noexcept {
            return path_.leaf();
        }

        TreeNodePtr as_mutable() const noexcept {
            return path_.leaf().as_mutable();
        }

        const TreeNodeConstPtr& node() const noexcept {
            return path_.leaf();
        }

        auto operator->() noexcept {
            return path_.leaf().operator->();
        }
    };


    class ConstNodeAccessor {
        const TreePathT& path_;
    public:
        ConstNodeAccessor(const TreePathT& path) noexcept:
            path_(path)
        {}

        operator const TreeNodeConstPtr&() const noexcept {return path_.leaf();}

        const auto operator->() const noexcept {
            return path_.leaf().operator->();
        }

        const TreeNodeConstPtr& node() const noexcept {
            return path_.leaf();
        }

        TreeNodePtr as_mutable() const noexcept {
            return path_.leaf().as_mutable();
        }
    };

    NodeAccessor iter_leaf() noexcept
    {
        return NodeAccessor(path_, self());
    }

    ConstNodeAccessor iter_leaf() const noexcept
    {
        return ConstNodeAccessor(path_);
    }


    const io::IOVector& iovector_view() const noexcept {
        return iovector_view_;
    }

    // TODO: error handling
    MEMORIA_V1_DECLARE_NODE_FN(RefreshIOVectorViewFn, configure_iovector_view);
    void refresh_iovector_view() noexcept
    {
        self().ctr().leaf_dispatcher().dispatch(path_.leaf(), RefreshIOVectorViewFn(), *&iovector_view_).get_or_throw();
    }


    IteratorCache& iter_cache() noexcept {
        return cache_;
    }

    const IteratorCache& iter_cache() const noexcept {
        return cache_;
    }


    bool iter_is_begin() const noexcept
    {
        return iter_local_pos() < 0 || iter_is_empty();
    }

    bool iter_is_end() const noexcept
    {
        auto& self = this->self();

        return iter_leaf().node().isSet() ? iter_local_pos() >= self.iter_leaf_size().get_or_throw() : true;
    }

    bool is_end() const noexcept
    {
        return self().iter_is_end();
    }

    bool iter_is_end(int32_t idx) const noexcept
    {
        auto& self = this->self();
        return iter_leaf().node().isSet() ? idx >= self.iter_leaf_size() : true;
    }

    bool iter_is_content() const noexcept
    {
        auto& self = this->self();
        return !(self.iter_is_begin() || self.iter_is_end());
    }

    bool iter_is_content(int32_t idx) const noexcept
    {
        auto& self = this->self();

        bool is_set = self.iter_leaf().node().isSet();

        auto iter_leaf_size = self.iter_leaf_size();

        return is_set && idx >= 0 && idx < iter_leaf_size;
    }

    bool iter_is_not_end() const noexcept
    {
        return !iter_is_end();
    }

    bool iter_is_empty() const noexcept
    {
        auto& self = this->self();
        return (iter_leaf().node().isEmpty()) || (self.iter_leaf_size() == 0);
    }

    bool iter_is_not_empty() const noexcept
    {
        return !iter_is_empty();
    }

    int64_t keyNum() const noexcept
    {
        return cache_.key_num();
    }

    int64_t& keyNum() noexcept
    {
        return cache_.key_num();
    }


    bool has_same_leaf(const Iterator& other) const noexcept
    {
        return self().iter_leaf()->id() == other.iter_leaf()->id();
    }


    VoidResult dump(std::ostream& out = std::cout, const char* header = nullptr) const noexcept
    {
        auto& self = this->self();

        out << (header != NULL ? header : self.iter_get_dump_header()) << std::endl;

        self.iter_dump_keys(out);
        self.iter_dump_cache(out);

        return self.iter_dump_blocks(out);
    }

    U8String iter_get_dump_header() const noexcept
    {
        return self().ctr().type_name_str() + " Iterator State";
    }

    VoidResult dumpPath(std::ostream& out = std::cout, const char* header = nullptr) const noexcept
    {
        auto& self  = this->self();
        out << (header != NULL ? header : self.iter_get_dump_header()) << std::endl;
        iter_dump_cache(out);
        iter_dump_keys(out);
        MEMORIA_TRY_VOID(self.ctr().ctr_dump_path(self.path(), 0, out));
        out << "======================================================================" << std::endl;

        return VoidResult::of();
    }

    void iter_dump_cache(std::ostream& out = std::cout) const noexcept
    {
        auto& self  = this->self();
        out << self.iter_cache() << std::endl;
    }

    void iter_dump_header(std::ostream& out = std::cout) const noexcept
    {
        self().iter_dump_cache(out);
        self().iter_dump_keys(out);
        if (self().iter_leaf().node().isSet()) {
            std::cout << "Node ID: " << self().iter_leaf()->id() << std::endl;
        }
    }

    void iter_dump_keys(std::ostream& out) const noexcept
    {
        auto& self = this->self();

        out << "Stream:  " << self.iter_data_stream_s() << std::endl;
        out << "Idx:  " << self.iter_local_pos() << std::endl;
    }



    VoidResult iter_dump_blocks(std::ostream& out) const noexcept
    {
        auto& self = this->self();
        return self.ctr().ctr_dump_node(self.iter_leaf(), out);
    }

    void iter_prepare() noexcept {}

    void iter_init() noexcept {}

    VoidResult iter_refresh() noexcept {
        return VoidResult::of();
    }

public:
    template <typename Walker>
    void iter_finish_walking(int32_t idx, const Walker& w, WalkCmd cmd) noexcept {}

MEMORIA_BT_ITERATOR_BASE_CLASS_END

}
