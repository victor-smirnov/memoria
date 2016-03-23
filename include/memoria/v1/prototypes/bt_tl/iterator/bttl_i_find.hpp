
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorFindName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::BranchNodeEntry                           BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT  = typename Container::Types::CtrSizeT;
    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;
    using IteratorBranchNodeEntry       = typename Container::Types::IteratorBranchNodeEntry;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams                = Container::Types::Streams;
    static const Int SearchableStreams      = Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;

protected:
    template <typename Walker>
    void finish_walking(Int idx, Walker& w, WalkCmd cmd) {
        Base::finish_walking(idx, w, cmd);
    }


    template <typename WWTypes>
    void finish_walking(Int idx, const FindForwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        if (cmd != WalkCmd::REFRESH)
        {
            constexpr Int Stream = FindForwardWalker<WWTypes>::Stream;

            auto& self = this->self();
            auto& cache = self.cache();

            auto& pos  = cache.data_pos();
            auto& size = cache.data_size();

            auto stream = Stream;

            auto start   = walker.branch_size_prefix_backup()[stream] + walker.idx_backup();
            auto current = walker.branch_size_prefix()[stream] + idx;

            pos[stream] += current - start;

            if (stream < Streams - 1)
            {
                pos[stream + 1] = 0;

                if (self.isContent(idx))
                {
                    size[stream + 1] = self.template idx_data_size<Stream>(idx);
                }
                else {
                    size[stream + 1] = -1;
                }
            }

            cache.abs_pos()[stream] = walker.branch_size_prefix()[stream] + idx;

            self.update_leaf_ranks(cmd);
        }
    }

    template <typename WWTypes>
    void finish_walking(Int idx, const FindBackwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        if (cmd != WalkCmd::REFRESH)
        {

            constexpr Int Stream = FindBackwardWalker<WWTypes>::Stream;

            auto& self = this->self();
            auto& cache = self.cache();

            auto& pos  = cache.data_pos();
            auto& size = cache.data_size();

            auto stream = Stream;

            auto start   = walker.branch_size_prefix_backup()[stream] + walker.idx_backup();
            auto current = walker.branch_size_prefix()[stream] + idx;

            if (pos[stream] == -1) pos[stream] = 0;
            pos[stream] -= start - current;

            if (stream < Streams - 1)
            {
                pos[stream + 1] = 0;

                if (self.isContent(idx))
                {
                    size[stream + 1] = self.template idx_data_size<Stream>(idx);
                }
                else {
                    size[stream + 1] = -1;
                }
            }

            cache.abs_pos()[stream] = walker.branch_size_prefix()[stream] + idx;

            self.update_leaf_ranks(cmd);
        }
    }


    template <typename WWTypes>
    void finish_walking(Int idx, const FindGEForwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
        if (cmd != WalkCmd::REFRESH)
        {
            constexpr Int Stream = FindGEForwardWalker<WWTypes>::Stream;

            auto& self = this->self();
            auto& cache = self.cache();

            auto& pos  = cache.data_pos();
            auto& size = cache.data_size();

            auto stream = self.stream();

            auto start   = walker.branch_size_prefix_backup()[stream] + walker.idx_backup();
            auto current = walker.branch_size_prefix()[stream] + idx;

            pos[stream] += current - start;

            if (stream < Streams - 1)
            {
                pos[stream + 1] = 0;

                if (self.isContent(idx))
                {
                    size[stream + 1] = self.template idx_data_size<Stream>(idx);
                }
                else {
                    size[stream + 1] = -1;
                }
            }

            cache.abs_pos()[stream] = walker.branch_size_prefix()[stream] + idx;

            self.update_leaf_ranks(cmd);
        }
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorFindName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
