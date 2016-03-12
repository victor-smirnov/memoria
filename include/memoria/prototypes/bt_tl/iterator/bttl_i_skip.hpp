
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTTL_ITER_SKIP_HPP
#define _MEMORIA_PROTOTYPES_BTTL_ITER_SKIP_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorSkipName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::BranchNodeEntry                               BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT  = typename Container::Types::CtrSizeT;
    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;
    using IteratorBranchNodeEntry       = typename Container::Types::IteratorBranchNodeEntry;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int StreamIdx>
    using LeafSizesSubstreamPath = typename Container::Types::template LeafSizesSubstreamPath<StreamIdx>;

    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams                = Container::Types::Streams;
    static const Int SearchableStreams      = Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;

    bool next() {
    	return self().skipFw(1) > 0;
    }

    bool prev() {
    	return self().skipBw(1) > 0;
    }

    struct SkipFwFn {
        template <Int StreamIdx, typename Itr>
        auto process(Itr&& iter, CtrSizeT n)
        {
            return iter.template skip_fw_<StreamIdx>(n);
        }
    };

    struct SkipBwFn {
        template <Int StreamIdx, typename Itr>
        auto process(Itr&& iter, CtrSizeT n)
        {
            return iter.template skip_bw_<StreamIdx>(n);
        }
    };


    CtrSizeT skipFw(CtrSizeT n)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto stream = self.stream();

        auto pos  = cache.data_pos()[stream];
        auto size = cache.data_size()[stream];

        CtrSizeT nn;

        if (pos + n > size) {
            nn = size - pos;
        }
        else {
            nn = n;
        }

        return bt::ForEachStream<Streams - 1>::process(stream, SkipFwFn(), self, nn);
    }


    CtrSizeT skipBw(CtrSizeT n)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto stream = self.stream();

        auto pos  = cache.data_pos()[stream];

        if (pos - n < 0) {
            n = pos;
        }

        return self.skipBw1(n);
    }

    CtrSizeT skipBw1(CtrSizeT n = 1)
    {
        auto& self = this->self();
        auto stream = self.stream();
        return bt::ForEachStream<Streams - 1>::process(stream, SkipBwFn(), self, n);
    }

    CtrSizeT skip(CtrSizeT n)
    {
        if (n > 0) {
            return skipFw(n);
        }
        else {
            return skipBw(n);
        }
    }

    CtrSizeT size() const {
        auto stream = self().stream();
        return self().cache().data_size()[stream];
    }

    CtrSizeT substream_size() const {
        auto& self = this->self();
        auto stream = self.stream();
        auto idx = self.idx();

        return self.get_substream_size(stream, idx);
    }

    CtrSizeT pos() const {
        auto stream = self().stream();
        return self().cache().data_pos()[stream];
    }


    CtrSizeT toData(CtrSizeT offset = 0)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto& stream = self.stream();

        auto& idx = self.idx();

        if (stream < SearchableStreams)
        {
            auto substream_offset   = self.count_items(stream, idx);
            auto stream_prefix      = self.data_offset(stream + 1);

            auto full_substream_offset = stream_prefix + substream_offset;

            cache.data_pos()[stream + 1] = -full_substream_offset;

            cache.data_size()[stream + 1] = self.get_substream_size(stream, idx);

            stream++;
            idx = 0;

            return self.skipFw(full_substream_offset + offset) - full_substream_offset;
        }
        else {
            return 0;
        }
    }



    void toIndex()
    {
        auto& self  = this->self();
        auto& cache = self.cache();

        auto& stream = self.stream();

        auto& idx = self.idx();

        if (stream > 0)
        {
            auto parent_idx = cache.abs_pos()[stream - 1];
            auto idx_prefix = cache.size_prefix()[stream - 1];

            cache.data_pos()[stream]    = -1;
            cache.data_size()[stream]   = -1;
            cache.abs_pos()[stream]     = -1;

            stream--;
            idx = 0;

            if (parent_idx >= idx_prefix)
            {
                auto offset = parent_idx - idx_prefix;

                cache.data_pos()[stream] -= offset;

                self.skipFw(offset);
            }
            else {
                auto delta = cache.size_prefix()[stream] - cache.abs_pos()[stream];

                cache.data_pos()[stream] += delta;
                self.skipBw(delta);
            }
        }
    }

    bool isSEnd() const
    {
        auto& self  = this->self();
        auto& cache = self.cache();

        auto& stream = self.stream();

        return cache.data_pos()[stream] >= cache.data_size()[stream];
    }

    Position positions_to() const
    {
        auto path = self().cache().data_pos();
        auto stream = self().stream();

        for (Int c = stream + 1; c < Streams; c++)
        {
            path[c] = -1;
        }

        return path;
    }

    Position sizes_of() const
    {
        auto sizes = self().cache().data_size();
        auto stream = self().stream();

        for (Int c = stream + 1; c < Streams; c++)
        {
            sizes[c] = -1;
        }

        return sizes;
    }


    Int local_parent_idx(Int stream, Int idx) const
    {
        MEMORIA_ASSERT(stream, >, 0);

        auto& self = this->self();

        Int excess = self.prefix_excess(stream);

        Int size = self.leaf_size(stream);

        if (idx >= excess && idx < size)
        {
            return self.find_offset(stream - 1, idx - excess);
        }
        else {
            return -1;
        }
    }

    Int local_child_idx(Int stream, Int idx) const
    {
        MEMORIA_ASSERT(stream, <, Streams - 1);

        auto& self = this->self();

        Int sum = self.count_items(stream, idx);

        Int excess = self.prefix_excess(stream + 1);

        return sum + excess;
    }


    Position leafrank_(Int pos) const
    {
        auto& self = this->self();

        LeafPrefixRanks ranks;
        self.ctr().compute_leaf_prefixes(self.leaf(), self.leaf_extent(), ranks);

        return self.ctr().leafrank_(self.leaf(), self.leaf_sizes(), ranks, pos);
    }


    Position leaf_extent() const
    {
        const auto& self  = this->self();
        const auto& cache = self.cache();
        const auto& branch_prefix = cache.prefixes();

        Position expected_sizes;
        bttl::detail::ExpectedSizesHelper<Streams - 1, LeafSizesSubstreamPath, AccumItemH>::process(branch_prefix, expected_sizes);

        expected_sizes[0] = cache.size_prefix()[0];

        return expected_sizes - cache.size_prefix();
    }


    void dumpRanks(std::ostream& out = std::cout) const
    {
        auto& self = this->self();

        LeafPrefixRanks ranks;

        self.ctr().compute_leaf_prefixes(self.leaf(), self.leaf_extent(), ranks);

        out<<"PrefixRanks:"<<endl;

        for (Int c = 0; c < Streams; c++) {
            out<<ranks[c]<<endl;
        }
    }

    void dumpExtent(std::ostream& out = std::cout) const
    {
        out<<"LeafExtent:"<<self().leaf_extent()<<endl;
    }



    Position adjust_to_indel()
    {
        auto& self = this->self();
        auto& cache = this->cache();

        auto size = self.size();
        auto pos  = self.pos();

        auto idx        = self.idx();
        auto stream     = self.stream();

        if (idx > 0 || pos < size || stream == 0)
        {
            return self._local_stream_posrank_(stream, idx);
        }
        else {
            auto abs_pos0            = cache.abs_pos()[stream];
            auto abs_pos             = cache.abs_pos()[stream - 1];
            auto abs_pos_leaf_prefix = cache.size_prefix()[stream - 1];

            auto local_pos           = abs_pos - abs_pos_leaf_prefix;

            if (local_pos < -1)
            {
                self._to_index(-1);
                return self._local_stream_pos_rank2(abs_pos0);
            }
            else {
                return self._local_stream_posrank_(stream, idx);
            }
        }
    }



// Internal API

private:

    void _to_index(CtrSizeT d)
    {
        auto& self  = this->self();
        auto& cache = self.cache();

        auto& stream = self.stream();

        auto& idx = self.idx();

        if (stream > 0)
        {
            auto parent_idx = cache.abs_pos()[stream - 1];
            auto idx_prefix = cache.size_prefix()[stream - 1];

            cache.data_pos()[stream]    = -1;
            cache.data_size()[stream]   = -1;
            cache.abs_pos()[stream]     = -1;

            stream--;
            idx = 0;

            if (parent_idx >= idx_prefix)
            {
                auto offset = parent_idx - idx_prefix;

                cache.data_pos()[stream] -= offset;

                self.skipFw(offset);
            }
            else {
                auto delta = cache.size_prefix()[stream] - cache.abs_pos()[stream];

                cache.data_pos()[stream] += (delta + d);
                self.skipBw(delta + d);
            }
        }
    }


    Position _local_stream_posrank_(Int stream, Int idx)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        Position ranks;

        ranks[stream] = idx;

        for (Int s = stream - 1; s >= 0; s--)
        {
            auto abs_pos             = cache.abs_pos()[s];
            auto abs_pos_leaf_prefix = cache.size_prefix()[s];

            auto pos = abs_pos - abs_pos_leaf_prefix;

            if (pos >= 0)
            {
                ranks[s] = pos + 1;
            }
            else {
                ranks[s] = 0;
            }
        }


        for (Int s = stream; s < SearchableStreams; s++)
        {
            ranks[s + 1] = self.local_child_idx(s, ranks[s]);
        }

        return ranks;
    }

    Position _local_stream_pos_rank2(CtrSizeT abs_pos)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto stream = self.stream();
        auto idx    = self.idx();

        Position ranks;
        ranks[stream] = idx;

        for (Int s = stream - 1; s >= 0; s--)
        {
            auto abs_pos = cache.abs_pos()[s];
            auto abs_pos_leaf_prefix = cache.size_prefix()[s];

            auto pos = abs_pos - abs_pos_leaf_prefix;

            if (pos >= 0)
            {
                ranks[s] = pos + 1;
            }
            else {
                ranks[s] = 0;
            }
        }

        auto sizes = self.leaf_sizes();

        for (Int s = stream + 1; s < Streams; s++)
        {
            ranks[s] = sizes[s];
        }

        self.stream()++;

        auto stream_pp = self.stream();

        self.idx() = sizes[stream_pp];
        cache.data_pos()[stream_pp] = 0;
        cache.abs_pos()[stream_pp]  = abs_pos;

        return ranks;
    }

public:



    Int find_offset(Int stream, Int idx) const
    {
        auto& self = this->self();

        return self.ctr().find_offset(self.leaf(), stream, idx);
    }

    struct DataOffsetFn {
        template <Int StreamIdx, typename Cache>
        auto process(Cache&& cache)
        {
            using Path = LeafSizesSubstreamPath<StreamIdx>;
            return AccumItemH<Path>::value(0, cache.prefixes());
        }
    };


    CtrSizeT data_offset(Int stream) const
    {
        auto& self  = this->self();
        auto& cache = self.cache();

        if (stream > 0)
        {
            auto expected_size = bt::ForEachStream<SearchableStreams - 1>::process(stream - 1, DataOffsetFn(), cache);

            MEMORIA_ASSERT(expected_size, >=, 0);

            auto actual_size = cache.size_prefix()[stream];

            return expected_size - actual_size;
        }
        else {
            return 0;
        }
    }

    CtrSizeT prefix_excess(Int stream) const {
        return self().data_offset(stream);
    }


    template <Int Stream>
    auto idx_data_size(Int idx) const
    {
        auto& self = this->self();
        return self.ctr().template _get_stream_counter<Stream>(self.leaf(), idx);
    }





    CtrSizeT count_items(Int stream, Int end) const
    {
        auto& self = this->self();
        return self.ctr().count_items(self.leaf(), stream, end);
    }




    struct GetSizeFn {
        template <Int Stream, typename Itr>
        static CtrSizeT process(Itr&& iter, Int stream, Int idx)
        {
            if (iter.isContent(idx))
            {
                return iter.template idx_data_size<Stream>(idx);
            }
            else {
                return -1;
            }
        }
    };

    struct GetSizeElseFn {
        template <Int Stream, typename Itr>
        static CtrSizeT process(Itr&& iter, Int stream, Int idx){
            return 0;
        }
    };

    template <Int Stream, typename... Args>
    CtrSizeT _get_substream_size(Args&&... args) const
    {
        return IfLess<Stream, SearchableStreams>::process(GetSizeFn(), GetSizeElseFn(), self(), std::forward<Args>(args)...);
    }


    struct GetSizeFn2 {
        template <Int StreamIdx, typename Iter, typename... Args>
        CtrSizeT process(Iter&& iter, Int stream, Args&&... args)
        {
            return GetSizeFn::template process<StreamIdx>(iter, stream, std::forward<Args>(args)...);
        }
    };


    template <typename... Args>
    CtrSizeT get_substream_size(Int stream, Args&&... args) const
    {
        return bt::ForEachStream<SearchableStreams - 1>::process(stream, GetSizeFn2(), self(), stream, std::forward<Args>(args)...);
    }



    template <typename... Args>
    SplitStatus add_substream_size(Int stream, Args&&... args)
    {
        auto& self = this->self();
        self.ctr().add_to_stream_counter(self.leaf(), stream, std::forward<Args>(args)...);
        return SplitStatus::NONE;
    }


    template <typename Walker>
    void finish_walking(Int idx, Walker& w, WalkCmd cmd) {
        Base::finish_walking(idx, w, cmd);
    }


    template <typename WTypes>
    void finish_walking(Int idx, const SkipForwardWalker<WTypes>& walker, WalkCmd cmd)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto& pos  = cache.data_pos();

        auto stream = self.stream();

        pos[stream] += walker.sum();

        cache.abs_pos()[stream] = walker.branch_size_prefix()[stream] + idx;

        self.update_leaf_ranks(cmd);
    }

    template <typename WTypes>
    void finish_walking(Int idx, const SkipBackwardWalker<WTypes>& walker, WalkCmd cmd)
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto& pos  = cache.data_pos();

        auto stream = self.stream();

        pos[stream] -= walker.sum();

        cache.abs_pos()[stream] = walker.branch_size_prefix()[stream] + idx;

        self.update_leaf_ranks(cmd);
    }

//    template <typename WTypes>
//    void finish_walking(Int idx, const ForwardLeafWalker<WTypes>& walker, WalkCmd cmd)
//    {
//      auto& self = this->self();
//      auto& cache = self.cache();
//
//      auto& pos  = cache.data_pos();
//
//      auto stream = self.stream();
//
//      pos[stream] += walker.sum();
//
//      cache.abs_pos()[stream] = walker.branch_size_prefix()[stream] + idx;
//
//      self.update_leaf_ranks(cmd);
//    }
//
//    template <typename WTypes>
//    void finish_walking(Int idx, const BackwardLeafWalker<WTypes>& walker, WalkCmd cmd)
//    {
//      auto& self = this->self();
//      auto& cache = self.cache();
//
//      auto& pos  = cache.data_pos();
//
//      auto stream = self.stream();
//
//      pos[stream] += walker.sum();
//
//      cache.abs_pos()[stream] = walker.branch_size_prefix()[stream] + idx;
//
//      self.update_leaf_ranks(cmd);
//    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorSkipName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif

