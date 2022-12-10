
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
#include <memoria/containers/collection/collection_entry_impl.hpp>

#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/tools/optional.hpp>

namespace memoria {

template <typename Types> class MultimapKeysChunkImpl;

template<typename Types>
class MultimapValuesChunkImpl:
        public Iter<typename Types::BlockIterStateTypes>,
        public MultimapValuesChunk<
            typename Types::KeyType,
            typename Types::ValueType,
            ApiProfile<typename Types::Profile>
        >
{
    using Base = Iter<typename Types::BlockIterStateTypes>;

protected:
    using Profile  = typename Types::Profile;
    using CtrSizeT = typename Types::CtrSizeT;
    using typename Base::TreePathT;

    using Key = typename Types::KeyType;
    using Value = typename Types::ValueType;

    using ValueView = DTTViewType<Value>;

    using ChunkT = MultimapValuesChunk<Key, Value, ApiProfile<Profile>>;
    using ChunkImplT = MultimapValuesChunkImpl;

    using KeysChunkT = MultimapKeysChunk<Key, Value, ApiProfile<Profile>>;
    using KeysChunkImplT = MultimapKeysChunkImpl<Types>;


    using ChunkPtr = IterSharedPtr<ChunkT>;
    using KeysChunkPtr = IterSharedPtr<KeysChunkT>;

    using ShuttleTypes = typename Types::ShuttleTypes;
    using Position = typename Types::Position;



    static constexpr size_t Stream = 1;
    static constexpr size_t Column = 0;


    using KeysPath = IntList<0, 1>;
    using ValuesPath = IntList<Stream, 1>;

    using SymsPath = IntList<Types::StructureStreamIdx, 1>;

    using Base::leaf_position_;

    size_t size_;
    bool before_start_ {};

    CtrSizeT leaf_run_start_{};
    CtrSizeT leaf_run_size_{};

    CtrSizeT run_offset_{};
    CtrSizeT run_size_{};

    mutable detail::SpanHolder<Value> span_holder_;
    mutable Optional<ValueView> view_;

    mutable LWMemHolder view_holder_;

protected:
    virtual void configure_refholder(SharedPtrHolder* owner) {
        view_holder_.set_owner(owner);
    }

public:

    void configure(const TreePathT& path, CtrSizeT value_off, CtrSizeT run_size)
    {
        Base::set_path(path);
        size_ = values_struct().size();
        run_size_ = run_size;
        leaf_run_start_ = value_off;
        leaf_position_ = value_off;
        run_offset_ = 0;

        update_run_size();
    }

    void update_run_size()
    {
        CtrSizeT run_remainder = run_size_ - run_offset_;

        if (leaf_run_start_ + run_remainder <= size_) {
            leaf_run_size_ = run_remainder;
        }
        else {
            leaf_run_size_ = size_ - leaf_run_start_;
        }
    }

    virtual CtrSizeT size() const {
        return run_size_;
    }

    virtual DTTConstPtr<Value> current_value() const
    {
        if (leaf_position_ < size_ && !before_start_) {
            if (!view_.is_initialized()) {
                init_current();
            }
            return DTTConstPtr<Value>(view_.get(), &view_holder_);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("EOF/BOF Exception: {} {}", size_, before_start_).do_throw();
        }
    }

    virtual CtrSizeT entry_offset() const {
        return run_offset_;
    }

    virtual CtrSizeT chunk_offset() const
    {
//        bt::GlobalLeafPrefixShuttle<ShuttleTypes, Stream> shuttle;
//        Base::ctr().ctr_ride_uptree(Base::path(), shuttle);
//        return shuttle.prefix();

        return CtrSizeT{};
    }

    virtual size_t chunk_size() const {
        return leaf_run_size_;
    }

    virtual size_t entry_offset_in_chunk() const {
        return leaf_position_ - leaf_run_start_;
    }

    virtual DTTConstSpan<Value> values() const {
        if (!span_holder_.set_up) {
            span_holder_.populate(values_struct(), Column, leaf_run_start_, leaf_run_size_, &view_holder_);
        }

        return DTTConstSpan<Value>(span_holder_.span, &view_holder_);
    }

    virtual bool is_before_start() const {
        return before_start_;
    }

    virtual bool is_after_end() const {
        return leaf_position_ >= size_ || run_offset_ >= run_size_;
    }

    virtual ChunkPtr next(CtrSizeT num = 1) const
    {
        CtrSizeT delta;

        if (run_offset_ + num <= run_size_) {
            delta = num;
        }
        else {
            delta = run_size_ - run_offset_;
        }

        using ShuttleT = bt::SkipForwardShuttle<ShuttleTypes, Stream, ChunkImplT>;
        auto next_ptr = Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, delta);

        next_ptr->run_offset_ += delta;
        next_ptr->before_start_ = false;

        if (leaf_position_ + delta >= size_) {
            next_ptr->leaf_run_start_ = 0;
        }

        next_ptr->update_run_size();

        return next_ptr;
    }



    virtual ChunkPtr prev(CtrSizeT num = 1) const
    {
        CtrSizeT delta;
        bool before_start;

        if (num <= run_offset_) {
            delta = num;
            before_start = false;
        }
        else {
            delta = run_offset_;
            before_start = true;
        }

        using ShuttleT = bt::SkipBackwardShuttle<ShuttleTypes, Stream, ChunkImplT>;
        auto prev_ptr = Base::ctr().ctr_ride_bw(this, TypeTag<ShuttleT>{}, delta);

        prev_ptr->run_offset_ -= delta;
        prev_ptr->before_start_ = before_start;

        return prev_ptr;
    }

    virtual ChunkPtr next_chunk() const
    {
        CtrSizeT remainder = (leaf_run_start_ + leaf_run_size_) - leaf_position_;
        return next(remainder);
    }

    virtual ChunkPtr prev_chunk() const {
        CtrSizeT prefix = leaf_position_ - leaf_run_start_;
        return prev(prefix);
    }

    virtual ChunkPtr read_to(DataTypeBuffer<Value>& buffer, CtrSizeT num) const {
        return ChunkPtr{};
    }

    void reset_state() noexcept
    {
        Base::reset_state();
        span_holder_.reset_state();
        view_.reset();
    }

    void prepare_ride(const ChunkImplT& src)
    {
        Base::prepare_ride(src);
        run_size_ = src.run_size_;
        run_offset_ = src.run_offset_;
    }

    void finish_ride(size_t pos, size_t size, bool before_start)
    {
        leaf_position_ = pos;
        size_ = size;
        before_start_ = before_start;
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



    virtual void iter_reset_caches()
    {
        if (leaf_position_ < size_ && !before_start_) {
            view_ = values_struct().access(0, leaf_position_);
            OwningViewSpanHelper<DTTViewType<Value>>::configure_resource_owner(view_.get(), &view_holder_);
        }
        else {
            view_.reset();
        }

        span_holder_.reset_state();
    }

    virtual bool is_found(const ValueView& key) const
    {
        if (leaf_position_ < size_ && !before_start_) {
            init_current();
            return view_.get() == key;
        }

        return false;
    }

    virtual KeysChunkPtr my_key() const {
        return KeysChunkPtr{};
    }

protected:
    void init_current() const {
        view_ = values_struct().access(Column, leaf_position_);
        OwningViewSpanHelper<DTTViewType<Value>>::configure_resource_owner(view_.get(), &view_holder_);
    }


    template <typename StreamPath>
    struct GetStructFn {
        template <typename LeafNodeSO>
        auto treeNode(const LeafNodeSO& node) {
            return node.template substream<StreamPath>();
        }
    };

    const auto values_struct() const {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetStructFn<ValuesPath>{});
    }

    const auto syms_struct() const {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetStructFn<SymsPath>{});
    }


    virtual Position iter_leafrank() const {
        auto ss = syms_struct();

        auto res = ss.select_fw_eq(leaf_position_, 1);
        auto key_pos = ss.rank_eq(res.idx, 0);

        return Position::create({key_pos, leaf_position_, res.idx});
    }

    void iter_finish_update(const Position& pos)
    {}
};

}
