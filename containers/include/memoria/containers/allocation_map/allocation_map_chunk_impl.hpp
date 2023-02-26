
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

#include <memoria/api/allocation_map/allocation_map_api.hpp>
#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/prototypes/bt/shuttles/bt_select_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_skip_shuttle.hpp>

#include <memoria/containers/allocation_map/allocation_map_shuttles.hpp>

namespace memoria {


template<typename Types>
class AllocationMapChunkImpl final:
        public Iter<typename Types::BlockIterStateTypes>,
        public AllocationMapChunk<ApiProfile<typename Types::Profile>>
{
    using Base = Iter<typename Types::BlockIterStateTypes>;
    using MyType = AllocationMapChunkImpl;
    using ChunkImpl = AllocationMapChunkImpl;

protected:
    using Profile = typename Types::Profile;
    using CtrSizeT = typename Types::CtrSizeT;

    using ChunkT = AllocationMapChunk<ApiProfile<Profile>>;
    using ChunkPtr = IterSharedPtr<ChunkT>;
    using ChunkImplPtr = IterSharedPtr<ChunkImpl>;

    using ShuttleTypes = typename Types::ShuttleTypes;
    using Position = typename Types::Position;

    using ApiProfileT = ApiProfile<Profile>;
    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;

    static constexpr size_t Stream = 0;

    using LeafPath = IntList<Stream, 1>;

public:
    using Base::leaf_position_;

protected:

    CtrSizeT size_;
    bool before_start_ {};

public:


    virtual CtrSizeT rank(size_t level) const
    {
        bt::AlcMapRankShuttle<ShuttleTypes, LeafPath> shuttle(leaf_position_, level);
        Base::ctr().ctr_ride_uptree(Base::path(), shuttle);
        return shuttle.rank();
    }

    virtual ChunkPtr select_fw(size_t level, CtrSizeT rank) const {
        return iter_select_fw(level, rank);
    }

    auto iter_select_fw(size_t level, CtrSizeT rank) const
    {
        using ShuttleT = bt::SelectForwardShuttle<ShuttleTypes, LeafPath, ChunkImpl>;
        return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, rank, level, SeqOpType::EQ);
    }


    virtual std::tuple<CtrSizeT, ChunkPtr> count_fw() const
    {
        CtrSizeT cnt;
        ChunkImplPtr next;
        std::tie(cnt, next) = iter_count_fw();
        return std::tuple<CtrSizeT, ChunkPtr>{cnt, next};
    }

    std::tuple<CtrSizeT, ChunkImplPtr> iter_count_fw() const
    {
        using ShuttleT = bt::CountForwardShuttle<ShuttleTypes, LeafPath, ChunkImpl>;
        ShuttleT shuttle(0);
        auto tgt = Base::ctr().ctr_ride_fw(this, shuttle);
        return std::tuple<CtrSizeT, ChunkImplPtr>{
            shuttle.sum(),
            tgt
        };
    }


    virtual CtrSizeT entry_offset() const {
        return chunk_offset() + entry_offset_in_chunk();
    }

    virtual CtrSizeT collection_size() const {
        return Base::ctr().size();
    }

    virtual CtrSizeT chunk_offset() const
    {
        bt::GlobalLeafPrefixShuttle<ShuttleTypes, Stream> shuttle;
        Base::ctr().ctr_ride_uptree(Base::path(), shuttle);
        return shuttle.prefix();
    }

    virtual size_t chunk_size() const {
        return size_;
    }

    virtual size_t entry_offset_in_chunk() const {
        return leaf_position_;
    }


    virtual bool is_before_start() const {
        return before_start_;
    }

    virtual bool is_after_end() const {
        return leaf_position_ >= size_;
    }

    virtual ChunkPtr next(CtrSizeT num = 1) const
    {
        return iter_next(num);
    }

    auto iter_next(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipForwardShuttle<ShuttleTypes, Stream, ChunkImpl>;
        return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual ChunkPtr next_chunk() const {
        return Base::ctr().ctr_next_leaf(this);
    }

    auto iter_next_chunk() const {
        return Base::ctr().ctr_next_leaf(this);
    }


    virtual ChunkPtr prev(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipBackwardShuttle<ShuttleTypes, Stream, ChunkImpl>;
        return Base::ctr().ctr_ride_bw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual ChunkPtr prev_chunk() const {
        return Base::ctr().ctr_prev_leaf(this);
    }

    void reset_state() noexcept
    {
        Base::reset_state();
        //span_ = Span<const RunT>{};
        //runs_.clear();
    }

    void finish_ride(size_t pos, size_t size, bool before_start)
    {
        leaf_position_ = pos;
        size_ = size;
        before_start_ = before_start;
    }

    virtual void dump(ChunkDumpMode mode = ChunkDumpMode::LEAF, std::ostream& out = std::cout) const
    {
        println(out, "Position: {}, size: {}, before_start: {}, id::{}", leaf_position_, size_, before_start_, Base::path().leaf()->id());
        if (mode == ChunkDumpMode::LEAF) {
            Base::ctr().ctr_dump_node(Base::path().leaf());
        }
        else if (mode == ChunkDumpMode::PATH) {
            Base::ctr().ctr_dump_path(Base::path(), 0);
        }
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

    virtual void iter_reset_caches()
    {
       // span_ = Span<const RunT>{};
        //runs_.clear();
    }



    void iter_clone_path()
    {
        Base::ctr().ctr_cow_clone_path(Base::path(), 0);
    }



    struct GetBitsFn {
        template <typename T>
        size_t treeNode(T&& node_so, int32_t level, int32_t pos) const noexcept
        {
            auto bitmap = node_so.template substream_by_idx<1>();
            size_t bm_size = bitmap.data()->size(level);
            (void)bm_size;
            MEMORIA_ASSERT(pos, <, bm_size);
            MEMORIA_ASSERT(pos, >=, 0);

            return bitmap.data()->get_bit(level, pos);
        }
    };

    size_t iter_get_bit(int32_t level, int32_t pos) const
    {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetBitsFn(), level, pos);
    }

    size_t iter_get_bit(int32_t level) const
    {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetBitsFn(), level, leaf_position_ >> level);
    }


    struct PopulateLeafFn {
        template <typename T>
        void treeNode(T&& node_so, Span<const AllocationMetadataT> leaf_allocations, bool set_bits, CtrSizeT base) const noexcept
        {
            auto bitmap = node_so.template substream<IntList<0, 1>>();

            if (set_bits) {
                for (auto alc: leaf_allocations) {
                    int32_t pos = (alc.position() - base) >> alc.level();
                    bitmap.set_bits(alc.level(), pos, alc.size_at_level());
                }
            }
            else {
                constexpr size_t Levels = std::remove_pointer_t<decltype(bitmap)>::LEVELS;
                bool levels[Levels]{false,};

                for (auto alc: leaf_allocations) {
                    int32_t pos = (alc.position() - base) >> alc.level();
                    bitmap.clear_bits_opt(alc.level(), pos, alc.size_at_level());
                    levels[alc.level()] = true;
                }

                for (size_t ll = 0; ll < Levels; ll++) {
                    if (levels[ll]) {
                        bitmap.rebuild_bitmaps(ll);
                    }
                }
            }

            return bitmap.reindex(false);
        }
    };

    void iter_populate_leaf(Span<const AllocationMetadataT> leaf_allocations, bool set_bits, CtrSizeT leaf_base)
    {
        Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), PopulateLeafFn(), leaf_allocations, set_bits, leaf_base);
        Base::ctr().ctr_update_path(Base::path(), 0);
    }

    virtual CtrSizeT level0_pos() const {
        return entry_offset();
    }

    virtual CtrSizeT leaf_size() const {
        return size_;
    }

    virtual const PkdAllocationMap<PkdAllocationMapTypes>* bitmap() const {
        return seq_struct().data();
    }


    virtual AnyID leaf_id() const {
        return AnyID::wrap(Base::path().leaf()->id());
    }

    virtual size_t current_bit(size_t level) const {
        size_t idx = leaf_position_ >> level;
        return seq_struct().data()->get_bit(level, idx);
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
};

}
