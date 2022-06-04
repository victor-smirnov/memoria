
// Copyright 2022 Victor Smirnov
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

#include <memoria/api/collection/collection_api.hpp>
#include <memoria/containers/collection/collection_shuttles.hpp>


#include <memoria/prototypes/bt/shuttles/bt_skip_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_rank_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_select_shuttle.hpp>
#include <memoria/prototypes/bt_fl/shuttles/btfl_structure_shuttles.hpp>

#include <memoria/core/tools/arena_buffer.hpp>

namespace memoria {




template<typename Types>
class BTFLStructureChunkImpl:
        public Iter<typename Types::BlockIterStateTypes>
{
    using Base = Iter<typename Types::BlockIterStateTypes>;

protected:
    using Profile = typename Types::Profile;
    using CtrSizeT = typename Types::CtrSizeT;
    using ShuttleTypes = typename Types::ShuttleTypes;
    using Position = typename Types::Position;
    using TreePathT = typename Types::TreePathT;

    static constexpr size_t Stream = Types::StructureStreamIdx;
    static constexpr size_t Column = 0;
    using LeafPath = IntList<Stream, 1>;


    static constexpr size_t AlphabetSize  = Stream;
    static constexpr size_t BitsPerSymbol = BitsPerSymbolConstexpr(AlphabetSize);

    using ChunkT    = BTFLStructureChunkImpl;
    using ChunkPtr  = IterSharedPtr<ChunkT>;

    using RunT      = SSRLERun<BitsPerSymbol>;
    using SymbolT   = typename RunT::SymbolT;

    CtrSizeT size_{};
    bool before_start_{};

    using Base::leaf_position_;

    mutable ArenaBuffer<RunT> runs_{};
    mutable Span<const RunT> span_{};

public:

    BTFLStructureChunkImpl() {}

    void configure(const TreePathT& path, size_t stream, CtrSizeT stream_position)
    {
        Base::set_path(path);
        auto ss = seq_struct();
        size_ = ss.size();

        auto res = ss.select_fw_eq(stream_position, stream);
        if (res.idx < size_) {
            leaf_position_ = res.idx;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Requested stream {} entry is not found in the BTFL structure sequence for position {}",
                        stream, stream_position
            ).do_throw();
        }
    }

    void configure(const TreePathT& path, CtrSizeT ss_position)
    {
        Base::set_path(path);
        auto ss = seq_struct();
        size_ = ss.size();

        leaf_position_ = ss_position;
    }

    const Span<const RunT>& runs() const
    {
        if (MMA_UNLIKELY(!runs_.size())) {
            seq_struct().iterator().read_to(runs_);
            span_ = runs_.span();
        }

        return span_;
    }

    SymbolT current_symbol() const {
        if (!before_start_ && leaf_position_ < size_) {
            return seq_struct().access(leaf_position_);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Range check in the Sequence chunk iterator: {} {} {}",
                        before_start_, leaf_position_, size_
            ).do_throw();
        }
    }

    CtrSizeT rank(SymbolT symbol, SeqOpType seq_op) const
    {
        bt::RankShuttle<ShuttleTypes, LeafPath> shuttle(leaf_position_, symbol, seq_op);
        Base::ctr().ctr_ride_uptree(Base::path(), shuttle);
        return shuttle.rank();
    }

    ChunkPtr select_fw(SymbolT symbol, CtrSizeT rank, SeqOpType seq_op) const
    {
        using ShuttleT = bt::SelectForwardShuttle<ShuttleTypes, LeafPath, ChunkT>;
        return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, rank, symbol, seq_op);
    }

    ChunkPtr select_bw(SymbolT symbol, CtrSizeT rank, SeqOpType seq_op) const {
        using ShuttleT = bt::SelectBackwardShuttle<ShuttleTypes, LeafPath, ChunkT>;
        return Base::ctr().ctr_ride_bw(this, TypeTag<ShuttleT>{}, rank, symbol, seq_op);
    }

    CtrSizeT entry_offset() const {
        return chunk_offset() + entry_offset_in_chunk();
    }

    CtrSizeT chunk_offset() const
    {
        bt::GlobalLeafPrefixShuttle<ShuttleTypes, Stream> shuttle;
        Base::ctr().ctr_ride_uptree(Base::path(), shuttle);
        return shuttle.prefix();
    }

    CtrSizeT chunk_size() const {
        return size_;
    }

    CtrSizeT entry_offset_in_chunk() const {
        return leaf_position_;
    }


    bool is_before_start() const {
        return before_start_;
    }

    bool is_after_end() const {
        return leaf_position_ >= size_;
    }

    ChunkPtr next(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipForwardShuttle<ShuttleTypes, Stream, ChunkT>;
        return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, num);
    }

    ChunkPtr next_chunk() const {
        return Base::ctr().ctr_next_leaf(this);
    }

    ChunkPtr prev(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipBackwardShuttle<ShuttleTypes, Stream, ChunkT>;
        return Base::ctr().ctr_ride_bw(this, TypeTag<ShuttleT>{}, num);
    }

    ChunkPtr prev_chunk() const {
        return Base::ctr().ctr_prev_leaf(this);
    }

    void reset_state() noexcept
    {
        Base::reset_state();
        span_ = Span<const RunT>{};
        runs_.clear();
    }

    void finish_ride(size_t pos, size_t size, bool before_start)
    {
        leaf_position_ = pos;
        size_ = size;
        before_start_ = before_start;
    }


    void dump(std::ostream& out) const
    {
        println(out, "Position: {}, size: {}, before_start: {}, id::{}", leaf_position_, size_, before_start_, Base::path().leaf()->id());
        Base::ctr().ctr_dump_node(Base::path().leaf());
    }


    EmptyType prepare_next_leaf() const {
        return EmptyType {};
    }

    EmptyType prepare_prev_leaf() const {
        return EmptyType {};
    }

    void on_next_leaf(EmptyType) {
        finish_ride(0, seq_struct().size(), false);
    }

    void on_prev_leaf(EmptyType) {
        finish_ride(0, seq_struct().size(), false);
    }

    void iter_reset_caches()
    {
        span_ = Span<const RunT>{};
        runs_.clear();
    }

    CtrSizeT read_to(CtrSizeT len, std::vector<RunT>& sink) const
    {
        CtrSizeT total{};
        read_to_(len, sink, total);

        if (len)
        {
            auto chunk = memoria_static_pointer_cast<ChunkT>(next_chunk());

            while (is_valid_chunk(chunk))
            {
                chunk->read_to_(len, sink, total);

                if (len) {
                    chunk = memoria_static_pointer_cast<ChunkT>(chunk->next_chunk());
                }
                else {
                    break;
                }
            }
        }

        return len;
    }

private:
    void read_to_(CtrSizeT& len, std::vector<RunT>& sink, CtrSizeT& total) const
    {
        CtrSizeT limit = (leaf_position_ + len <= size_) ? len : (size_ - leaf_position_);

        auto rr = seq_struct().symbol_runs(leaf_position_, limit);
        sink.insert(sink.end(), rr.begin(), rr.end());

        auto num = count_symbols_in(to_const_span(rr));
        total += num;

        len -= limit;
    }

protected:
    struct GetStructFn {
        template <typename LeafNodeSO>
        auto treeNode(const LeafNodeSO& node) {
            return node.template substream<LeafPath>();
        }
    };

    const auto seq_struct() const {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetStructFn{});
    }

    virtual Position iter_leafrank() const {
        MEMORIA_MAKE_GENERIC_ERROR("Not implemented").do_throw();
    }

    void iter_finish_update(const Position& pos) {
        MEMORIA_MAKE_GENERIC_ERROR("Not implemented").do_throw();
    }
};

}
