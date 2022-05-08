
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
#include <memoria/prototypes/bt/shuttles/bt_skip_shuttle.hpp>

#include <memoria/core/tools/arena_buffer.hpp>

namespace memoria {

namespace detail {

template <typename KeyDT, bool FixedSizeKey = DTTIsNDFixedSize<KeyDT>>
struct SpanHolder {
    using ViewType = DTTViewType<KeyDT>;

    ArenaBuffer<ViewType> arena;

    Span<const ViewType> span;
    bool set_up{};

    template <typename PkdStruct>
    void populate(const PkdStruct& ss, size_t column)
    {
        auto ee = ss.end(column);

        for (auto ii = ss.begin(column); ii != ee; ii++) {
            arena.append_value(*ii);
        }

        span = arena.span();

        set_up = true;
    }

    void reset_state() {
        set_up = false;
        span = Span<ViewType>{};
        arena.clear();
    }
};

template <typename KeyDT>
struct SpanHolder<KeyDT, true> {
    using ViewType = DTTViewType<KeyDT>;

    Span<const ViewType> span;
    bool set_up{};

    template <typename PkdStruct>
    void populate(const PkdStruct& ss, size_t column)
    {
        span = ss.span(column);
        set_up = true;
    }

    void reset_state() {
        set_up = false;
        span = Span<ViewType>{};
    }
};


}

template<typename Types>
class CollectionEntryImpl: 
        public Iter<typename Types::BlockIterStateTypes>,
        public CollectionEntry<typename Types::CollectionKeyType, ApiProfile<typename Types::Profile>>
{
    using Base = Iter<typename Types::BlockIterStateTypes>;

protected:
    using Profile = typename Types::Profile;
    using CtrSizeT = typename Types::CtrSizeT;

    using Key = typename Types::CollectionKeyType;

    using KeyView = DTTViewType<Key>;

    using EntryT = CollectionEntry<Key, ApiProfile<Profile>>;
    using EntryIterSharedPtr = IterSharedPtr<EntryT>;
    using ShuttleTypes = typename Types::ShuttleTypes;

    static constexpr size_t Stream = 0;
    static constexpr size_t Column = 0;

    using LeafPath = IntList<Stream, 1>;

    size_t position_{};
    size_t size_;
    bool before_start_ {};

    mutable detail::SpanHolder<Key> span_holder_;

    KeyView view_;



public:

    virtual const KeyView& value() const {
        if (position_ < size_ && !before_start_) {
            return view_;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("EOF/BOF Exception: {} {}", size_, before_start_).do_throw();
        }
    }

    virtual Datum<Key> read_value() const {
        return Datum<Key>{};
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
        return position_;
    }

    virtual const Span<const KeyView>& chunk() const {
        if (!span_holder_.set_up) {
            span_holder_.populate(keys_struct(), Column);
        }

        return span_holder_.span;
    }

    virtual bool is_before_start() const {
        return before_start_;
    }

    virtual bool is_after_end() const {
        return position_ >= size_;
    }

    virtual EntryIterSharedPtr next(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipForwardShuttle<ShuttleTypes, Stream, CollectionEntryImpl>;
        return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual EntryIterSharedPtr next_chunk() const {
        return Base::ctr().ctr_next_leaf(this);
    }

    virtual EntryIterSharedPtr prev(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipBackwardShuttle<ShuttleTypes, Stream, CollectionEntryImpl>;
        return Base::ctr().ctr_ride_bw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual EntryIterSharedPtr prev_chunk() const {
        return Base::ctr().ctr_prev_leaf(this);
    }

    virtual EntryIterSharedPtr read_to(DataTypeBuffer<Key>& buffer, CtrSizeT num) const {
        return EntryIterSharedPtr{};
    }

    void reset_state() noexcept
    {
        Base::reset_state();
        span_holder_.reset_state();
    }

    void set_position(size_t pos, size_t size, bool before_start = false)
    {
        position_ = pos;
        size_ = size;
        before_start_ = before_start;

        if (position_ < size_ && !before_start_) {
            view_ = keys_struct().access(0, position_);
        }
    }

    virtual void dump(std::ostream& out)
    {
        println(out, "Position: {}, size: {}, before_start: {}, id::{}", position_, size_, before_start_, Base::path().leaf()->id());
    }

    void on_next_leaf() {
        set_position(0, keys_struct().size());
    }

    void on_prev_leaf() {
        set_position(0, keys_struct().size());
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
