
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
#include <memoria/containers/collection/collection_entry_impl.hpp>
#include <memoria/containers/map/map_shuttles.hpp>

#include <memoria/prototypes/bt/shuttles/bt_skip_shuttle.hpp>

#include <memoria/core/tools/arena_buffer.hpp>

namespace memoria {



template<typename Types>
class MapChunkImpl:
        public Iter<typename Types::BlockIterStateTypes>,
        public MapChunk<
            typename Types::KeyType,
            typename Types::ValueType,
            ApiProfile<typename Types::Profile>
        >
{
    using Base = Iter<typename Types::BlockIterStateTypes>;
    using ChunkImpl = MapChunkImpl;

protected:
    using Profile = typename Types::Profile;
    using CtrSizeT = typename Types::CtrSizeT;

    using Key   = typename Types::KeyType;
    using Value = typename Types::ValueType;

    using KeyView = DTTViewType<Key>;
    using ValueView = DTTViewType<Value>;

    using EntryT = MapChunk<Key, Value, ApiProfile<Profile>>;
    using EntryIterSharedPtr = IterSharedPtr<EntryT>;
    using ShuttleTypes = typename Types::ShuttleTypes;
    using Position = typename Types::Position;

    static constexpr size_t Stream = 0;
    static constexpr size_t Column = 0;

    using KeysLeafPath   = IntList<Stream, 1>;
    using ValuesLeafPath = IntList<Stream, 2>;

    using Base::leaf_position_;

    size_t size_;
    bool before_start_ {};

    mutable detail::SpanHolder<Key> keys_holder_;
    mutable detail::SpanHolder<Value> values_holder_;

    KeyView key_view_;
    ValueView value_view_;

    mutable ViewPtrHolder view_holder_;

protected:
    virtual void configure_refholder(SharedPtrHolder* owner) {
        view_holder_.set_owner(owner);
    }

public:

    virtual DTTConstPtr<Key> current_key() const {
        if (leaf_position_ < size_ && !before_start_) {
            return DTTConstPtr<Key>(key_view_, &view_holder_);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("EOF/BOF Exception: {} {}", size_, before_start_).do_throw();
        }
    }

    virtual DTTConstPtr<Value> current_value() const {
        if (leaf_position_ < size_ && !before_start_) {
            return DTTConstPtr<Value>(value_view_, &view_holder_);
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
        if (!keys_holder_.set_up) {
            keys_holder_.populate(keys_struct(), Column, &view_holder_);
        }

        return DTTConstSpan<Key>(keys_holder_.span, &view_holder_);
    }

    virtual DTTConstSpan<Value> values() const {
        if (!values_holder_.set_up) {
            values_holder_.populate(values_struct(), Column, &view_holder_);
        }

        return DTTConstSpan<Value>(values_holder_.span, &view_holder_);
    }

    virtual bool is_before_start() const {
        return before_start_;
    }

    virtual bool is_after_end() const {
        return leaf_position_ >= size_;
    }

    virtual EntryIterSharedPtr next(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipForwardShuttle<ShuttleTypes, Stream, ChunkImpl>;
        return Base::ctr().ctr_ride_fw(this, TypeTag<ShuttleT>{}, num);
    }

    virtual EntryIterSharedPtr next_chunk() const {
        return Base::ctr().ctr_next_leaf(this);
    }

    virtual EntryIterSharedPtr prev(CtrSizeT num = 1) const
    {
        using ShuttleT = bt::SkipBackwardShuttle<ShuttleTypes, Stream, ChunkImpl>;
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
        keys_holder_.reset_state();
        values_holder_.reset_state();
    }

    void finish_ride(size_t pos, size_t size, bool before_start = false)
    {
        leaf_position_ = pos;
        size_ = size;
        before_start_ = before_start;

        if (leaf_position_ < size_ && !before_start_) {
            key_view_ = keys_struct().access(0, leaf_position_);
            OwningViewSpanHelper<DTTViewType<Key>>::configure_resource_owner(key_view_, &view_holder_);

            value_view_ = values_struct().access(0, leaf_position_);
            OwningViewSpanHelper<DTTViewType<Value>>::configure_resource_owner(value_view_, &view_holder_);
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
            key_view_ = keys_struct().access(0, leaf_position_);
            OwningViewSpanHelper<DTTViewType<Key>>::configure_resource_owner(key_view_, &view_holder_);

            value_view_ = values_struct().access(0, leaf_position_);
            OwningViewSpanHelper<DTTViewType<Value>>::configure_resource_owner(value_view_, &view_holder_);
        }
        else {
            key_view_ = KeyView{};
            value_view_ = ValueView{};
        }

        keys_holder_.reset_state();
        values_holder_.reset_state();
    }

    virtual bool is_found(const KeyView& key) const
    {
        if (leaf_position_ < size_ && !before_start_) {
            return key_view_ == key;
        }

        return false;
    }

protected:

    template <typename LeafStructPath>
    struct GetStructFn {
        template <typename LeafNodeSO>
        auto treeNode(const LeafNodeSO& node) {
            return node.template substream<LeafStructPath>();
        }
    };

    const auto keys_struct() const {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetStructFn<KeysLeafPath>{});
    }

    const auto values_struct() const {
        return Base::ctr().leaf_dispatcher().dispatch(Base::path().leaf(), GetStructFn<ValuesLeafPath>{});
    }
};

}
