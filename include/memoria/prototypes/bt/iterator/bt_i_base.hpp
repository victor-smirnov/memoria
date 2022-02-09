
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
    BTIteratorBase():
        Base(), idx_(0), stream_(0)
    {
    }

    BTIteratorBase(ThisType&& other):
        Base(std::move(other)),
        path_(std::move(other.path_)),
        idx_(other.idx_),
        stream_(other.stream_),
        cache_(std::move(other.cache_))
    {
        cache_.init(&self());
    }

    BTIteratorBase(const ThisType& other):
        Base(other),
        path_(other.path_),
        idx_(other.idx_),
        stream_(other.stream_),
        cache_(other.cache_)
    {
    }


    void assign(ThisType&& other)
    {
        path_       = std::move(other.path_);
        idx_        = other.idx_;
        stream_     = other.stream_;
        cache_      = std::move(other.cache_);

        refresh_iovector_view();

        Base::assign(std::move(other));
    }

    void assign(const ThisType& other)
    {
        path_       = other.path_;
        idx_        = other.idx_;
        stream_     = other.stream_;

        cache_      = other.cache_;

        refresh_iovector_view();

        Base::assign(other);
    }

    TreePathT& path() {
        return path_;
    }

    const TreePathT& path() const {
        return path_;
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
        TreePathT& path_;

        MyType& iter_;

    public:
        NodeAccessor(TreePathT& path, MyType& iter) :
            path_(path), iter_(iter)
        {}

        operator TreeNodeConstPtr&()  {return path_.leaf();}
        operator const TreeNodeConstPtr&() const  {return path_.leaf();}

        void assign(const TreeNodeConstPtr& node)
        {
            path_.leaf() = node;
            iter_.refresh_iovector_view();
        }

        TreeNodeConstPtr& node()  {
            return path_.leaf();
        }

        TreeNodePtr as_mutable() const  {
            return path_.leaf().as_mutable();
        }

        const TreeNodeConstPtr& node() const  {
            return path_.leaf();
        }

        auto operator->()  {
            return path_.leaf().operator->();
        }
    };


    class ConstNodeAccessor {
        const TreePathT& path_;
    public:
        ConstNodeAccessor(const TreePathT& path) :
            path_(path)
        {}

        operator const TreeNodeConstPtr&() const  {return path_.leaf();}

        const auto operator->() const  {
            return path_.leaf().operator->();
        }

        const TreeNodeConstPtr& node() const  {
            return path_.leaf();
        }

        TreeNodePtr as_mutable() const  {
            return path_.leaf().as_mutable();
        }
    };

    NodeAccessor iter_leaf()
    {
        return NodeAccessor(path_, self());
    }

    ConstNodeAccessor iter_leaf() const
    {
        return ConstNodeAccessor(path_);
    }


    const io::IOVector& iovector_view() const  {
        return iovector_view_;
    }

    // TODO: error handling
    MEMORIA_V1_DECLARE_NODE_FN(RefreshIOVectorViewFn, configure_iovector_view);
    void refresh_iovector_view()
    {
        self().ctr().leaf_dispatcher().dispatch(path_.leaf(), RefreshIOVectorViewFn(), *&iovector_view_);
    }


    IteratorCache& iter_cache()  {
        return cache_;
    }

    const IteratorCache& iter_cache() const  {
        return cache_;
    }


    bool iter_is_begin() const
    {
        return iter_local_pos() < 0 || iter_is_empty();
    }

    bool iter_is_end() const
    {
        auto& self = this->self();

        return iter_leaf().node().isSet() ? iter_local_pos() >= self.iter_leaf_size() : true;
    }

    bool is_end() const
    {
        return self().iter_is_end();
    }

    bool iter_is_end(int32_t idx) const
    {
        auto& self = this->self();
        return iter_leaf().node().isSet() ? idx >= self.iter_leaf_size() : true;
    }

    bool iter_is_content() const
    {
        auto& self = this->self();
        return !(self.iter_is_begin() || self.iter_is_end());
    }

    bool iter_is_content(int32_t idx) const
    {
        auto& self = this->self();

        bool is_set = self.iter_leaf().node().isSet();

        auto iter_leaf_size = self.iter_leaf_size();

        return is_set && idx >= 0 && idx < iter_leaf_size;
    }

    bool iter_is_not_end() const
    {
        return !iter_is_end();
    }

    bool iter_is_empty() const
    {
        auto& self = this->self();
        return (iter_leaf().node().isEmpty()) || (self.iter_leaf_size() == 0);
    }

    bool iter_is_not_empty() const
    {
        return !iter_is_empty();
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

        out << (header != NULL ? header : self.iter_get_dump_header()) << std::endl;

        self.iter_dump_keys(out);
        self.iter_dump_cache(out);

        return self.iter_dump_blocks(out);
    }

    U8String iter_get_dump_header() const
    {
        return self().ctr().type_name_str() + " Iterator State";
    }

    void dumpPath(std::ostream& out = std::cout, const char* header = nullptr) const
    {
        auto& self  = this->self();
        out << (header != NULL ? header : self.iter_get_dump_header()) << std::endl;
        iter_dump_cache(out);
        iter_dump_keys(out);
        self.ctr().ctr_dump_path(self.path(), 0, out);
        out << "======================================================================" << std::endl;
    }

    void iter_dump_cache(std::ostream& out = std::cout) const
    {
        auto& self  = this->self();
        out << self.iter_cache() << std::endl;
    }

    void iter_dump_header(std::ostream& out = std::cout) const
    {
        self().iter_dump_cache(out);
        self().iter_dump_keys(out);
        if (self().iter_leaf().node().isSet()) {
            std::cout << "Node ID: " << self().iter_leaf()->id() << std::endl;
        }
    }

    void iter_dump_keys(std::ostream& out) const
    {
        auto& self = this->self();

        out << "Stream:  " << self.iter_data_stream_s() << std::endl;
        out << "Idx:  " << self.iter_local_pos() << std::endl;
    }



    void iter_dump_blocks(std::ostream& out) const
    {
        auto& self = this->self();
        return self.ctr().ctr_dump_node(self.iter_leaf(), out);
    }

    void iter_prepare() {}

    void iter_init() {}

    void iter_refresh() {
    }

public:
    template <typename Walker>
    void iter_finish_walking(int32_t idx, const Walker& w, WalkCmd cmd) {}

MEMORIA_BT_ITERATOR_BASE_CLASS_END

}
