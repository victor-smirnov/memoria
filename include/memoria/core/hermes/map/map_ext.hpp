
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

#include <memoria/core/hermes/map/object_map.hpp>
#include <memoria/core/hermes/map/typed_map.hpp>
#include <memoria/core/hermes/container.hpp>

namespace memoria {
namespace hermes {

inline void MapView<Varchar, Object>::assert_mutable()
{
    if (MMA_UNLIKELY(!mem_holder_->is_mem_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("MapView<String, Object> is immutable").do_throw();
    }
}

template <typename KeyDT>
inline void MapView<KeyDT, Object>::assert_mutable()
{
    if (MMA_UNLIKELY(!mem_holder_->is_mem_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("MapView<String, Object> is immutable").do_throw();
    }
}


template <typename DT>
inline ObjectMap MapView<Varchar, Object>::put_dataobject(U8StringView key, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    auto key_ptr = ctr.new_dataobject<Varchar>(key);
    auto value_ptr = ctr.new_embeddable_dataobject<DT>(value);

    auto arena = ctr.arena();
    MapStorageT* new_map;

    ShortTypeCode mytag = arena::read_type_tag(map_);
    if (value_ptr.get_vs_tag() == VS_TAG_ADDRESS) {
        new_map = map_->put(*arena, mytag,
                            reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(key_ptr.addr()),
                            value_ptr.storage_.addr);
    }
    else if (value_ptr.get_vs_tag() == VS_TAG_SMALL_VALUE) {
        new_map = map_->put(*arena, mytag,
                            reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(key_ptr.addr()),
                            value_ptr.storage_.small_value.to_eptr());
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Invalid value type").do_throw();
    }

    return ObjectMap(mem_holder_, new_map);
}

template <typename KeyDT>
template <typename DT>
inline Map<KeyDT, Object> MapView<KeyDT, Object>::put_dataobject(KeyView key, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    HermesCtr ctr = HermesCtr(mem_holder_);
    auto value_ptr = ctr.new_embeddable_dataobject<DT>(value);
    auto arena = ctr.arena();

    void* new_map;
    ShortTypeCode mytag = arena::read_type_tag(map_);

    if (value_ptr.get_vs_tag() == VS_TAG_ADDRESS) {
        new_map = map_->put(*arena, mytag, key, value_ptr.storage_.addr);
    }
    else if (value_ptr.get_vs_tag() == VS_TAG_SMALL_VALUE) {
        new_map = map_->put(*arena, mytag, key, value_ptr.storage_.small_value.to_eptr());
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Invalid value type").do_throw();
    }

    return Map<KeyDT, Object>(mem_holder_, new_map);
}



inline ObjectMap MapView<Varchar, Object>::remove(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    ShortTypeCode mytag = arena::read_type_tag(map_);
    MapStorageT* new_map = map_->remove(*(ctr.arena()), mytag, key);

    return ObjectMap(mem_holder_, new_map);
}

template <typename KeyDT>
inline Map<KeyDT, Object> MapView<KeyDT, Object>::remove(KeyView key)
{
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(mem_holder_);
    ShortTypeCode mytag = arena::read_type_tag(map_);
    auto* new_map = map_->remove(*(ctr.arena()), mytag, key);
    return Map<KeyDT, Object>(mem_holder_, new_map);
}


inline void MapView<Varchar, Object>::do_stringify(std::ostream& out, DumpFormatState& state) const
{
    auto& spec = state.cfg().spec();

    if (size() > 0)
    {
        out << "{" << spec.nl_start();

        bool first = true;

        state.push();
        for_each([&](auto kk, auto vv){
            if (MMA_LIKELY(!first)) {
                out << "," << spec.nl_middle();
            }
            else {
                first = false;
            }

            state.make_indent(out);

            if (state.cfg().use_raw_strings()) {
                U8StringView kk_escaped = RawStringEscaper::current().escape_quotes(kk);
                out << "'" << kk_escaped << "':" << spec.space();
                RawStringEscaper::current().reset();
            }
            else {
                U8StringView kk_escaped = StringEscaper::current().escape_chars(kk);
                out << "\"" << kk_escaped << "\":" << spec.space();
                RawStringEscaper::current().reset();
            }

            vv.stringify(out, state);
        });
        state.pop();

        out << spec.nl_end();

        state.make_indent(out);
        out << "}";
    }
    else {
        out << "{}";
    }
}

template <typename KeyDT>
inline void MapView<KeyDT, Object>::do_stringify(std::ostream& out, DumpFormatState& state) const
{
    auto& spec = state.cfg().spec();

    out << "<";
    out << get_datatype_name(type_to_str<KeyDT>());
    out << "," << spec.space();
    out << "Object";
    out << ">" << spec.space();

    if (size() > 0)
    {
        out << "{" << spec.nl_start();

        bool first = true;

        state.push();
        for_each([&](auto kk, auto vv){
            if (MMA_LIKELY(!first)) {
                out << "," << spec.nl_middle();
            }
            else {
                first = false;
            }

            state.make_indent(out);

            arena::ArenaDataTypeContainer<KeyDT>::stringify_view(out, state, kk);

            out << ":" << spec.space();

            vv.stringify(out, state);
        });
        state.pop();

        out << spec.nl_end();

        state.make_indent(out);
        out << "}";
    }
    else {
        out << "{}";
    }
}

inline ObjectMap MapView<Varchar, Object>::put_object(U8StringView name, const Object& value) {
    assert_not_null();
    assert_mutable();

    void* new_map;
    auto ctr = HermesCtr(mem_holder_);
    auto vv = ctr.do_import_value(value);
    if (!vv.is_null())
    {
        ShortTypeCode mytag = arena::read_type_tag(map_);
        auto arena = ctr.arena();
        auto key = ctr.new_dataobject<Varchar>(name);
        new_map = map_->put(*arena, mytag,
                            reinterpret_cast<arena::ArenaDataTypeContainer<Varchar>*>(key.addr()),
                            vv.storage_.addr);
    }
    else {
        return remove(name);
    }

    return ObjectMap(mem_holder_, new_map);
}

template <typename KeyDT>
inline Map<KeyDT, Object> MapView<KeyDT, Object>::put_object(KeyView key, const Object& value) {
    assert_not_null();
    assert_mutable();

    auto ctr = HermesCtr(this->get_mem_holder());
    void* new_map;
    auto vv = ctr.do_import_value(value);
    if (!vv.is_null())
    {
        ShortTypeCode mytag = arena::read_type_tag(map_);
        auto arena = ctr.arena();
        new_map = map_->put(*arena, mytag, key, vv.storage_.addr);
    }
    else {
        return remove(key);
    }

    return Map<KeyDT, Object>(mem_holder_, new_map);
}

inline PoolSharedPtr<GenericMap> MapView<Varchar, Object>::as_generic_map() const {
    return TypedGenericMap<Varchar, Object>::make_wrapper(self());
}



template <typename KeyDT>
PoolSharedPtr<GenericMap> TypedGenericMap<KeyDT, Object>::make_wrapper(
        Map<KeyDT, Object>&& map
) {
    using GMPoolT = pool::SimpleObjectPool<TypedGenericMap<KeyDT, Object>>;
    using GMPoolPtrT = boost::local_shared_ptr<GMPoolT>;

    static thread_local GMPoolPtrT wrapper_pool = MakeShared<GMPoolT>();
    return wrapper_pool->allocate_shared(std::move(map));
}



template <typename KeyDT>
PoolSharedPtr<GenericMapEntry> TypedGenericMap<KeyDT, Object>::iterator() const
{
    using GMEntryPoolT = pool::SimpleObjectPool<TypedGenericMapEntry<KeyDT, Object>>;
    using GMEntryPoolPtrT = boost::local_shared_ptr<GMEntryPoolT>;

    static thread_local GMEntryPoolPtrT entry_pool = MakeShared<GMEntryPoolT>();
    return entry_pool->allocate_shared(map_.begin(), map_.end());
}

template <typename KeyDT>
HermesCtr MapView<KeyDT, Object>::ctr() const {
    assert_not_null();
    return HermesCtr(this->get_mem_holder());
}

template <typename KeyDT>
HermesCtr TypedGenericMap<KeyDT, Object>::ctr() const {
    return map_.ctr();
}

}}
