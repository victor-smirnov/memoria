
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

#include <memoria/api/multimap/multimap_api.hpp>
#include <memoria/containers/collection/collection_shuttles.hpp>
#include <memoria/containers/collection/collection_entry_impl.hpp>

#include <memoria/prototypes/bt/shuttles/bt_skip_shuttle.hpp>
#include <memoria/core/tools/arena_buffer.hpp>

#include <memoria/prototypes/bt_fl/btfl_structure_chunk_iter.hpp>
#include <memoria/containers/multimap/multimap_values_chunk_impl.hpp>

namespace memoria {


template<typename Types>
class MultimapKeysChunkImpl:
        public Iter<typename Types::BlockIterStateTypes>,
        public MultimapKeysChunk<
            typename Types::KeyType,
            typename Types::ValueType,
            ApiProfile<typename Types::Profile>
        >
{
    using Base = Iter<typename Types::BlockIterStateTypes>;

protected:
    using Profile = typename Types::Profile;
    using CtrSizeT = typename Types::CtrSizeT;

    using Key   = typename Types::KeyType;
    using Value = typename Types::ValueType;

    using KeyView = DTTViewType<Key>;

    using ChunkT = MultimapKeysChunk<Key, Value, ApiProfile<Profile>>;
    using ChunkImplT = MultimapKeysChunkImpl;

    using ValuesChunkT = MultimapValuesChunk<Key, Value, ApiProfile<Profile>>;
    using ValuesChunkImplT = MultimapValuesChunkImpl<Types>;

    using ChunkPtr = IterSharedPtr<ChunkT>;
    using ValuesChunkPtr = IterSharedPtr<ValuesChunkT>;

    using StructureChunkImplT = BTFLStructureChunkImpl<Types>;
    using StructureChunkPtr   = IterSharedPtr<StructureChunkImplT>;

    using ShuttleTypes = typename Types::ShuttleTypes;
    using Position = typename Types::Position;

    static constexpr size_t Stream = 0;
    static constexpr size_t Column = 0;

    using KeysPath = IntList<Stream, 1>;
    using SymsPath = IntList<Types::StructureStreamIdx, 1>;

    using Base::leaf_position_;

    CtrSizeT size_;
    bool before_start_ {};

    mutable detail::SpanHolder<Key> span_holder_;

    KeyView view_;

    mutable DTViewHolder view_holder_;

protected:
    virtual void configure_refholder(pool::detail::ObjectPoolRefHolder* owner) {
        view_holder_.set_owner(owner);
    }

public:

    virtual DTTConstPtr<Key> current_key() const {
        if (leaf_position_ < size_ && !before_start_) {
            return DTTConstPtr<Key>(view_, &view_holder_);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("EOF/BOF Exception: {} {}", size_, before_start_).do_throw();
        }
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

    virtual DTTConstSpan<Key> keys() const {
        if (!span_holder_.set_up) {
            span_holder_.populate(keys_struct(), Column, &view_holder_);
        }

        return DTTConstSpan<Key>(span_holder_.span, &view_holder_);
    }

    virtual bool is_before_start() const {
        return before_start_;
    }

    virtual bool is_after_end() const {
        return leaf_position_ >= size_;
    }

    virtual ChunkPtr next(CtrSizeT num = 1) const
    {
        //using ShuttleT = bt::SkipForwardShuttle<ShuttleTypes, Stream, ChunkImplT>;
        //return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, num);
        return iter_next(num);
    }

    auto iter_next(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipForwardShuttle<ShuttleTypes, Stream, ChunkImplT>;
        return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual ChunkPtr next_chunk() const {
        return Base::ctr().ctr_next_leaf(this);
    }

    virtual ChunkPtr prev(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipBackwardShuttle<ShuttleTypes, Stream, ChunkImplT>;
        return Base::ctr().ctr_ride_bw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual ChunkPtr prev_chunk() const {
        return Base::ctr().ctr_prev_leaf(this);
    }

    virtual ChunkPtr read_to(DataTypeBuffer<Key>& buffer, CtrSizeT num) const {
        return ChunkPtr{};
    }

    void reset_state() noexcept
    {
        Base::reset_state();
        span_holder_.reset_state();
    }

    void finish_ride(size_t pos, size_t size, bool before_start = false)
    {
        leaf_position_ = pos;
        size_ = size;
        before_start_ = before_start;

        if (leaf_position_ < size_ && !before_start_) {
            view_ = keys_struct().access(0, leaf_position_);
            OwningViewSpanHelper<DTTViewType<Key>>::configure_resource_owner(view_, &view_holder_);
        }
    }

    virtual void dump(ChunkDumpMode mode, std::ostream& out) const
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
        finish_ride(0, keys_struct().size(), false);
    }

    void on_prev_leaf(EmptyType) {
        finish_ride(0, keys_struct().size(), false);
    }

    virtual void iter_reset_caches()
    {
        if (leaf_position_ < size_ && !before_start_) {
            view_ = keys_struct().access(0, leaf_position_);
            OwningViewSpanHelper<DTTViewType<Key>>::configure_resource_owner(view_, &view_holder_);
        }
        else {
            view_ = KeyView{};
        }

        span_holder_.reset_state();
    }

    virtual bool is_found(const KeyView& key) const
    {
        if (leaf_position_ < size_ && !before_start_) {
            return view_ == key;
        }

        return false;
    }

    virtual ValuesChunkPtr values_chunk() const {
        return values_chunk(leaf_position_);
    }

    virtual ValuesChunkPtr values_chunk(size_t idx) const
    {
        if (idx < size_ && !before_start_)
        {
            auto ss = syms_struct();
            auto res = ss.select_fw_eq(idx, 0);
            auto ss_size = ss.size();
            if (res.idx < ss_size)
            {
                StructureChunkPtr ss_chunk = make_ss_chunk_iter(res.idx);
                StructureChunkPtr ss_next_key = ss_chunk->select_fw(0, 1, SeqOpType::EQ);

                CtrSizeT key_off = ss_chunk->entry_offset();
                CtrSizeT next_key_off = ss_next_key->entry_offset();
                CtrSizeT span_size = next_key_off - key_off - 1;

                auto ptr = Base::ctr().make_block_iterator_state(TypeTag<ValuesChunkImplT>{});

                if (res.idx < ss_size - 1)
                {
                    CtrSizeT value_off = ss.rank_eq(res.idx + 1, 1);
                    ptr->configure(Base::path(), value_off, span_size);
                }
                else {
                    StructureChunkPtr ss_next_val = ss_chunk->select_fw(1, 0, SeqOpType::EQ);
                    ptr->configure(ss_next_val->path(), 0, span_size);
                }

                return ptr;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Can't select key's {} position in the structure stream", idx).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in MuntimapKeysChunk::values_chunk(): {} {}", idx, size_).do_throw();
        }
    }

protected:

    StructureChunkPtr make_ss_chunk_iter(CtrSizeT ss_idx) const
    {
        auto ptr = Base::ctr().make_block_iterator_state(TypeTag<StructureChunkImplT>{});
        ptr->configure(Base::path(), ss_idx);
        return ptr;
    }

    ValuesChunkPtr make_values_chunk_iter(CtrSizeT ss_idx, CtrSizeT span_size) const
    {
        auto ptr = Base::ctr().make_block_iterator_state(TypeTag<ValuesChunkImplT>{});
        ptr->configure(Base::path(), ss_idx, span_size);
        return ptr;
    }

    template <typename StreamPath>
    struct GetStructFn {
        template <typename LeafNodeSO>
        auto treeNode(const LeafNodeSO& node) {
            return node.template substream<StreamPath>();
        }
    };

    const auto keys_struct() const {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetStructFn<KeysPath>{});
    }

    const auto syms_struct() const {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetStructFn<SymsPath>{});
    }

    virtual Position iter_leafrank() const {
        auto ss = syms_struct();

        auto res = ss.select_fw_eq(leaf_position_, 0);
        auto value_pos = ss.rank_eq(res.idx, 1);

        return Position::create({leaf_position_, value_pos, res.idx});
    }

    void iter_finish_update(const Position& pos)
    {}
};

}
