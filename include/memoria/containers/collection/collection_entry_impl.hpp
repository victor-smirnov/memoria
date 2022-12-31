
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

#include <memoria/core/tools/arena_buffer.hpp>

#include <memoria/core/hermes/hermes.hpp>

namespace memoria {

namespace detail {

template <typename KeyDT, bool FixedSizeKey = DTTIsNDFixedSize<KeyDT>>
struct SpanHolder;

template <typename KeyDT>
struct SpanHolder<KeyDT, false> {
    using StorageT = typename DataTypeTraits<KeyDT>::SpanStorageT;

    std::vector<StorageT> arena;

    bool set_up{};

    template <typename PkdStruct>
    void populate(const PkdStruct& ss, size_t column, LWMemHolder* owner)
    {
        auto ee = ss.end(column);

        for (auto ii = ss.begin(column); ii != ee; ii++) {
            arena.push_back(*ii);
        }

        set_up = true;
    }


    template <typename PkdStruct>
    void populate(const PkdStruct& ss, size_t column, size_t start, size_t size, LWMemHolder* owner)
    {
        auto ee = ss.end(column);

        for (auto ii = ss.begin(column); ii != ee; ii++) {
            arena.push_back(*ii);
        }

        set_up = true;
    }

    DTSpan<KeyDT> span(LWMemHolder* owner) const {
        return DTSpan<KeyDT>(owner, Span<const StorageT>{arena.data(), arena.size()});
    }

    void reset_state() {
        set_up = false;
        arena.erase(arena.begin(), arena.end());
    }
};



template <typename KeyDT>
struct SpanHolder<KeyDT, true> {
    using StorageT = typename DataTypeTraits<KeyDT>::SpanStorageT;

    Span<const StorageT> span_;
    bool set_up{};

    template <typename PkdStruct>
    void populate(const PkdStruct& ss, size_t column, LWMemHolder* owner)
    {
        span_ = ss.span(column);
        set_up = true;
    }

    template <typename PkdStruct>
    void populate(const PkdStruct& ss, size_t column, size_t start, size_t size, LWMemHolder* owner)
    {
        span_ = ss.span(column).subspan(start, size);
        set_up = true;
    }

    DTSpan<KeyDT> span(LWMemHolder* owner) const {
        return DTSpan<KeyDT>(owner, Span<const StorageT>{});
    }

    void reset_state() {
        set_up = false;
        span_ = Span<const StorageT>{};
    }
};


template <>
struct SpanHolder<Hermes, false> {
    using StorageT = typename DataTypeTraits<Hermes>::SpanStorageT;

    std::vector<StorageT> arena;

    bool set_up{};

    template <typename PkdStruct>
    void populate(const PkdStruct& ss, size_t column, LWMemHolder* owner)
    {
        auto ee = ss.end(column);

        for (auto ii = ss.begin(column); ii != ee; ii++) {
            arena.emplace_back(owner->owner());
        }

        set_up = true;
    }


    template <typename PkdStruct>
    void populate(const PkdStruct& ss, size_t column, size_t start, size_t size, LWMemHolder* owner)
    {
        auto ee = ss.end(column);

        for (auto ii = ss.begin(column); ii != ee; ii++)
        {
            arena.emplace_back(owner->owner());
        }

        set_up = true;
    }

    DTSpan<Hermes> span(LWMemHolder* owner) const {
        return DTSpan<Hermes>(owner, Span<const StorageT>{arena.data(), arena.size()});
    }

    void reset_state() {
        set_up = false;
        arena.erase(arena.begin(), arena.end());
    }
};



}




template<typename Types>
class CollectionChunkImpl:
        public Iter<typename Types::BlockIterStateTypes>,
        public CollectionChunk<typename Types::CollectionKeyType, ApiProfile<typename Types::Profile>>
{
    using Base = Iter<typename Types::BlockIterStateTypes>;

protected:
    using Profile = typename Types::Profile;
    using CtrSizeT = typename Types::CtrSizeT;

    using Key = typename Types::CollectionKeyType;

    using KeyView = DTTViewType<Key>;

    using EntryT = CollectionChunk<Key, ApiProfile<Profile>>;
    using EntryIterSharedPtr = IterSharedPtr<EntryT>;
    using ShuttleTypes = typename Types::ShuttleTypes;
    using Position = typename Types::Position;

    static constexpr size_t Stream = 0;
    static constexpr size_t Column = 0;

    using LeafPath = IntList<Stream, 1>;

    using Base::leaf_position_;

    size_t size_;
    bool before_start_ {};

    mutable detail::SpanHolder<Key> span_holder_;

    KeyView view_;

    mutable LWMemHolder view_holder_;

protected:
    virtual void configure_refholder(SharedPtrHolder* owner) {
        view_holder_.set_owner(owner);
    }

public:

    virtual DTView<Key> current_key() const {
        if (leaf_position_ < size_ && !before_start_) {
            return DTView<Key>(&view_holder_, view_);
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

    virtual DTSpan<Key> keys() const {
        if (!span_holder_.set_up) {
            span_holder_.populate(keys_struct(), Column, &view_holder_);
        }

        return span_holder_.span(&view_holder_);
    }

    virtual bool is_before_start() const {
        return before_start_;
    }

    virtual bool is_after_end() const {
        return leaf_position_ >= size_;
    }

    virtual EntryIterSharedPtr next(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipForwardShuttle<ShuttleTypes, Stream, CollectionChunkImpl>;
        return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual EntryIterSharedPtr next_chunk() const {
        return Base::ctr().ctr_next_leaf(this);
    }

    virtual EntryIterSharedPtr prev(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipBackwardShuttle<ShuttleTypes, Stream, CollectionChunkImpl>;
        return Base::ctr().ctr_ride_bw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual EntryIterSharedPtr prev_chunk() const {
        return Base::ctr().ctr_prev_leaf(this);
    }

    virtual EntryIterSharedPtr read_to(HermesDTBuffer<Key>& buffer, CtrSizeT num) const {
        return EntryIterSharedPtr{};
    }

    void reset_state() noexcept
    {
        Base::reset_state();
        span_holder_.reset_state();
    }

    void finish_ride(size_t pos, size_t size, bool before_start)
    {
        leaf_position_ = pos;
        size_ = size;
        before_start_ = before_start;

        if (leaf_position_ < size_ && !before_start_) {
            view_ = keys_struct().access(0, leaf_position_);
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

protected:
    struct GetStructFn {
        template <typename LeafNodeSO>
        auto treeNode(const LeafNodeSO& node) {
            return node.template substream<LeafPath>();
        }
    };

    const auto keys_struct() const {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetStructFn{});
    }
};

}
