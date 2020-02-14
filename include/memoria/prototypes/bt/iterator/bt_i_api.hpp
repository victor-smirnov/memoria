
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
#include <memoria/core/container/macros.hpp>

#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt/walkers/bt_misc_walkers.hpp>

#include <iostream>


namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(bt::IteratorAPIName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::BranchNodeEntry                           BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Types::Position                                            Position;

    typedef typename Container::Types::CtrSizeT                                 CtrSizeT;

    using typename Base::TreePathT;

    static const int32_t Streams = Container::Types::Streams;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SizesFn, sizes, Position);

public:



    auto iter_leaf_sizes() const {
        return self().ctr().leaf_dispatcher().dispatch(self().iter_leaf(), SizesFn());
    }

    VoidResult iter_refresh() noexcept
    {
        MEMORIA_TRY_VOID(Base::iter_refresh());

        MEMORIA_TRY_VOID(self().iter_refresh_branch_prefixes());
        self().iter_refresh_leaf_prefixes();

        return VoidResult::of();
    }
    

    BoolResult iter_next_leaf() noexcept;
    BoolResult iter_next_leaf_ms(uint64_t streams) noexcept;


    BoolResult iter_prev_leaf() noexcept;

    bool iter_is_found() noexcept {
        auto& self = this->self();
        return (!self.iter_is_end()) && self.iter_is_not_empty();
    }

    void iter_dump_keys(std::ostream& out) const noexcept
    {
        Base::iter_dump_keys(out);
    }

    Result<CtrSizeT> iter_skip_stream_fw(int32_t stream, CtrSizeT distance) noexcept;
    Result<CtrSizeT> iter_skip_stream_bw(int32_t stream, CtrSizeT distance) noexcept;
    Result<CtrSizeT> iter_skip_stream(int32_t stream, CtrSizeT distance) noexcept;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SizeFn, size, int32_t);

    int32_t iter_leaf_size(int32_t stream) const noexcept
    {
        return self().iter_leaf_size0(stream);
    }

    int32_t iter_leaf_size0(int32_t stream) const noexcept
    {
        return self().ctr().leaf_dispatcher().dispatch(self().iter_leaf(), SizeFn(), stream);
    }

    int32_t iter_leaf_size() const noexcept
    {
        return self().ctr().leaf_dispatcher().dispatch(self().iter_leaf(), SizeFn(), self().iter_stream());
    }


    bool iter_is_leaf_empty() const noexcept
    {
        return self().model().ctr_is_node_empty(self().iter_leaf());
    }

    int32_t iter_leaf_capacity(int32_t stream) const noexcept
    {
        auto& self = this->self();
        return self.iter_leaf_capacity(Position(), stream);
    }

    int32_t iter_leaf_capacity(const Position& reserved, int32_t stream) const noexcept
    {
        auto& self = this->self();
        auto& ctr = self.model();

        return ctr.getStreamCapacity(self.iter_leaf(), reserved, stream);
    }

    template <typename Walker>
    BoolResult iter_find_next_leaf(Walker&& walker) noexcept;

    template <typename Walker>
    BoolResult iter_find_prev_leaf(Walker&& walker) noexcept;

    VoidResult iter_create_empty_leaf() noexcept
    {
        auto& self = this->self();
        auto& ctr  = self.model();

        TreePathT left_path  = self.path();
        TreePathT right_path = self.path();

        auto next = ctr.ctr_split_leaf(left_path, right_path, self.iter_leaf(), self.iter_leaf_sizes());

        self.path() = right_path;
        self.iter_leaf().assign(next);

        self.iter_local_pos() = 0;
    }

    template <int32_t Stream, typename SubstreamsList, typename... Args>
    VoidResult iter_update_stream(Args&&... args) noexcept
    {
        auto& self = this->self();
        auto& ctr  = self.model();

        ctr.template ctr_update_stream_entry<Stream, SubstreamsList>(self, std::make_tuple(std::forward<Args>(args)...));
    }


    template <int32_t Stream, typename SubstreamsIdxList, typename... Args>
    auto iter_read_leaf_entry(Args&&... args) const noexcept
    {
         return self().ctr().template ctr_apply_substreams_fn<Stream, SubstreamsIdxList>(self().iter_leaf(), bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }

    template <typename Walker>
    VoidResult iter_walk_up_for_refresh(const TreePathT& path, size_t level, int32_t idx, Walker&& walker) const noexcept
    {
        self().ctr().node_dispatcher().dispatch(path[level], walker, WalkCmd::PREFIXES, 0, idx);

        for (size_t ll = level + 1; ll < path.size(); ll++)
        {
            MEMORIA_TRY(child_idx, self().ctr().ctr_get_child_idx(path[ll], path[ll - 1]->id()));
            self().ctr().branch_dispatcher().dispatch(path[ll], walker, WalkCmd::PREFIXES, 0, child_idx);
        }

        return VoidResult::of();
    }

    template <typename Walker>
    VoidResult iter_walk_up_for_refresh(TreePathT& path, size_t level, int32_t idx, Walker&& walker) noexcept
    {
        self().ctr().node_dispatcher().dispatch(path[level], walker, WalkCmd::PREFIXES, 0, idx);

        for (size_t ll = level + 1; ll < path.size(); ll++)
        {
            MEMORIA_TRY(child_idx, self().ctr().ctr_get_child_idx(path[ll], path[ll - 1]->id()));
            self().ctr().branch_dispatcher().dispatch(path[ll], walker, WalkCmd::PREFIXES, 0, child_idx);
        }

        return VoidResult::of();
    }

    VoidResult iter_refresh_branch_prefixes() noexcept
    {
        auto& self  = this->self();
        auto& iter_cache = self.iter_cache();

        bt::FindForwardWalker<bt::WalkerTypes<Types, IntList<0>>> walker(0, 0);

        iter_cache.reset();

        MEMORIA_TRY_VOID(self.iter_walk_up_for_refresh(self.path(), 0, self.iter_local_pos(), walker));

        walker.finish(self, self.iter_local_pos(), WalkCmd::REFRESH);

        return VoidResult::of();
    }

    template <int StreamIdx>
    void iter_refresh_stream_leaf_prefixes() noexcept
    {
        auto& self  = this->self();
        auto idx    = self.iter_local_pos();

        bt::FindForwardWalker<bt::WalkerTypes<Types, IntList<StreamIdx>>> walker(0, 0);
        self.ctr().leaf_dispatcher().dispatch(self.iter_leaf(), walker, WalkCmd::LAST_LEAF, 0, idx);
    }


    struct RefreshLeafPrefixesFn
    {
        template <int32_t StreamIdx, typename Iter>
        bool process(Iter&& iter)
        {
            if (StreamIdx == iter.iter_stream())
            {
                iter.template iter_refresh_stream_leaf_prefixes<StreamIdx>();
            }

            return true;
        }
    };


    void iter_refresh_leaf_prefixes() noexcept
    {
        auto& self  = this->self();
        ForEach<1, Streams>::process(RefreshLeafPrefixesFn(), self);
    }


    


MEMORIA_V1_ITERATOR_PART_END


#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(bt::IteratorAPIName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

// --------------------- PUBLIC API --------------------------------------


M_PARAMS
Result<typename M_TYPE::CtrSizeT> M_TYPE::iter_skip_stream_fw(int32_t stream, CtrSizeT amount) noexcept
{
    //using ResultT = Result<CtrSizeT>;
    typedef typename Types::template SkipForwardWalker<Types> Walker;

    auto& self = this->self();

    Walker walker(stream, 0, amount);

    walker.prepare(self);

    int32_t idx = self.model().findFw(self.iter_leaf(), stream, self.idx(    ), walker);

    return walker.finish(self, idx);
}

M_PARAMS
Result<typename M_TYPE::CtrSizeT> M_TYPE::iter_skip_stream_bw(int32_t stream, CtrSizeT amount) noexcept
{
    //using ResultT = Result<CtrSizeT>;
    typedef typename Types::template SkipBackwardWalker<Types> Walker;

    auto& self = this->self();

    Walker walker(stream, 0, amount);

    walker.prepare(self);

    int32_t idx = self.model().findBw(self.iter_leaf(), stream, self.iter_local_pos(), walker);

    return walker.finish(self, idx);
}




M_PARAMS
Result<typename M_TYPE::CtrSizeT> M_TYPE::iter_skip_stream(int32_t stream, CtrSizeT amount) noexcept
{
    //using ResultT = Result<CtrSizeT>;
    auto& self = this->self();

    if (amount > 0)
    {
        return self.iter_skip_stream_fw(stream, amount);
    }
    else if (amount < 0) {
        return self.iter_skip_stream_bw(stream, -amount);
    }
    else {
        return 0;
    }
}

M_PARAMS
BoolResult M_TYPE::iter_next_leaf_ms(uint64_t streams) noexcept
{
    typedef typename Types::template NextLeafMutistreamWalker<Types, IntList<0>, IntList<0>> Walker;

    auto& self = this->self();

    Walker walker(streams);

    return self.iter_find_next_leaf(walker);
}


M_PARAMS
BoolResult M_TYPE::iter_next_leaf() noexcept
{
    typedef typename Types::template NextLeafWalker<Types, IntList<0>> Walker;
    Walker walker(self().iter_stream(), 0);

    return self().iter_find_next_leaf(walker);
}




M_PARAMS
BoolResult M_TYPE::iter_prev_leaf() noexcept
{
    typedef typename Types::template PrevLeafWalker<Types, IntList<0>> Walker;

    auto& self = this->self();
    int32_t stream = self.iter_stream();

    Walker walker(stream, 0);

    return self.iter_find_prev_leaf(walker);
}



M_PARAMS
template <typename Walker>
BoolResult M_TYPE::iter_find_next_leaf(Walker&& walker) noexcept
{
    auto& self = this->self();

    NodeBaseG&  leaf    = self.iter_leaf();
    int32_t stream      = self.iter_stream();

    if (!leaf->is_root())
    {
        walker.prepare(self);

        NodeBaseG parent = self.ctr().ctr_get_node_parent(leaf);

        int32_t idx = self.ctr().findFw(parent, stream, leaf->parent_idx() + 1, walker);

        int32_t size = self.ctr().ctr_get_node_size(parent, stream);

        MEMORIA_ASSERT_RTN(size, >, 0);

        int32_t child_idx;

        if (idx < size)
        {
            child_idx = idx;
        }
        else {
            child_idx = size - 1;
        }

        // Step down the tree
        leaf = self.ctr().ctr_get_node_child(parent, child_idx);

        walker.finish(self, idx < size);

        self.iter_local_pos() = 0;

        return BoolResult::of(idx < size);
    }
    else {
        return BoolResult::of(false);
    }
}



M_PARAMS
template <typename Walker>
BoolResult M_TYPE::iter_find_prev_leaf(Walker&& walker) noexcept
{
    auto& self = this->self();

    NodeBaseG&  leaf    = self.iter_leaf();
    int32_t stream      = self.iter_stream();

    if (!leaf->is_root())
    {
        walker.prepare(self);

        NodeBaseG parent = self.ctr().ctr_get_node_parent(leaf);

        int32_t idx = self.model().findBw(parent, stream, leaf->parent_idx() - 1, walker);

        int32_t size = self.model().ctr_get_node_size(parent, stream);

        MEMORIA_ASSERT_RTN(size, >, 0);

        int32_t child_idx;

        if (idx >= 0)
        {
            child_idx = idx;
        }
        else {
            child_idx = 0;
        }

        // Step down the tree
        leaf = self.model().ctr_get_node_child(parent, child_idx);

        walker.finish(self, idx >= 0);

        self.iter_local_pos() = idx >= 0 ? self.iter_leaf_size(stream) - 1 : -1;

        return BoolResult::of(idx >= 0);
    }
    else {
        return BoolResult::of(false);
    }
}


#undef M_TYPE
#undef M_PARAMS


}
