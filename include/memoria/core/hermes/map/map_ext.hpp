
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

#include <memoria/core/hermes/map/map.hpp>
#include <memoria/core/hermes/map/typed_map.hpp>
#include <memoria/core/hermes/container.hpp>

namespace memoria {
namespace hermes {

inline void Map<Varchar, Object>::assert_mutable()
{
    if (MMA_UNLIKELY(!doc_->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Map<String, Object> is immutable").do_throw();
    }
}

template <typename KeyDT>
inline void Map<KeyDT, Object>::assert_mutable()
{
    if (MMA_UNLIKELY(!doc_->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Map<String, Object> is immutable").do_throw();
    }
}


template <typename DT>
inline DataObjectPtr<DT> Map<Varchar, Object>::put_dataobject(U8StringView key, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    auto key_ptr = doc_->new_dataobject<Varchar>(key);
    auto value_ptr = doc_->new_dataobject<DT>(value);

    auto arena = doc_->arena();
    map_->put(*arena, key_ptr->dt_ctr(), value_ptr->value_storage_.addr);

    return value_ptr;
}

template <typename KeyDT>
template <typename DT>
inline DataObjectPtr<DT> Map<KeyDT, Object>::put_dataobject(KeyView key, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    auto key_ptr = doc_->new_dataobject<Varchar>(key);
    auto value_ptr = doc_->new_dataobject<DT>(value);

    auto arena = doc_->arena();
    map_->put(*arena, key_ptr->dt_ctr(), value_ptr->value_storage_.addr);

    return value_ptr;
}



inline ObjectMapPtr Map<Varchar, Object>::put_generic_map(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    auto key_ptr = doc_->new_dataobject<Varchar>(key);
    auto value_ptr = doc_->new_map();

    auto arena = doc_->arena();
    map_->put(*arena, key_ptr->dt_ctr(), value_ptr->map_);

    return value_ptr;
}

inline ObjectArrayPtr Map<Varchar, Object>::put_generic_array(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    auto key_ptr = doc_->new_dataobject<Varchar>(key);
    auto value_ptr = doc_->new_array();

    auto arena = doc_->arena();
    map_->put(*arena, key_ptr->dt_ctr(), value_ptr->array_);

    return value_ptr;
}


inline void Map<Varchar, Object>::remove(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    map_->remove(*(doc_->arena()), key);
}

template <typename KeyDT>
inline void Map<KeyDT, Object>::remove(KeyView key)
{
    assert_not_null();
    assert_mutable();

    map_->remove(*(doc_->arena()), key);
}


inline void Map<Varchar, Object>::do_stringify(std::ostream& out, DumpFormatState& state) const
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

            vv->stringify(out, state);
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
inline void Map<KeyDT, Object>::do_stringify(std::ostream& out, DumpFormatState& state) const
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

            DataObject<KeyDT>::stringify_view(out, state, kk);

            out << ":" << spec.space();

            vv->stringify(out, state);
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


inline void Map<Varchar, Object>::put(StringValuePtr name, ObjectPtr value) {
    assert_not_null();
    assert_mutable();

    if (!value->is_null()) {
        auto arena = doc_->arena();
        map_->put(*arena, name->dt_ctr(), value->value_storage_.addr);
    }
}

inline void Map<Varchar, Object>::put(U8StringView name, ObjectPtr value) {
    assert_not_null();
    assert_mutable();

    auto vv = doc_->do_import_value(value);
    if (!vv->is_null())
    {
        auto arena = doc_->arena();
        auto key = doc_->new_dataobject<Varchar>(name);
        map_->put(*arena, key->dt_ctr(), vv->value_storage_.addr);
    }
}

template <typename KeyDT>
inline void Map<KeyDT, Object>::put(KeyView key, ObjectPtr value) {
    assert_not_null();
    assert_mutable();

    auto vv = doc_->do_import_value(value);
    if (!vv->is_null())
    {
        auto arena = doc_->arena();
        map_->put(*arena, key, vv->value_storage_.addr);
    }
}

inline ObjectPtr Map<Varchar, Object>::put_hermes(U8StringView key, U8StringView str) {
  assert_not_null();
  assert_mutable();

  auto key_ptr = doc_->new_dataobject<Varchar>(key);
  auto value_ptr = doc_->parse_raw_value(str.begin(), str.end());
  auto vv1 = doc_->do_import_value(value_ptr);

  auto arena = doc_->arena();
  map_->put(*arena, key_ptr->dt_ctr(), vv1->value_storage_.addr);

  return vv1;
}

template <typename KeyDT>
inline ObjectPtr Map<KeyDT, Object>::put_hermes(KeyView key, U8StringView str) {
  assert_not_null();
  assert_mutable();

  auto value_ptr = doc_->parse_raw_value(str.begin(), str.end());
  auto vv = doc_->do_import_value(value_ptr);

  auto arena = doc_->arena();
  map_->put(*arena, key, vv->value_storage_.addr);

  return vv;
}

inline PoolSharedPtr<GenericMap> Map<Varchar, Object>::as_generic_map() const {
    return TypedGenericMap<Varchar, Object>::make_wrapper(map_, doc_, ptr_holder_);
}



template <typename KeyDT>
PoolSharedPtr<GenericMap> TypedGenericMap<KeyDT, Object>::make_wrapper(
        void* array, HermesCtr* ctr, ViewPtrHolder* ctr_holder
) {
    using GMPoolT = pool::SimpleObjectPool<TypedGenericMap<KeyDT, Object>>;
    using GMPoolPtrT = boost::local_shared_ptr<GMPoolT>;

    static thread_local GMPoolPtrT wrapper_pool = MakeShared<GMPoolT>();
    return wrapper_pool->allocate_shared(array, ctr, ctr_holder);
}



template <typename KeyDT>
PoolSharedPtr<GenericMapEntry> TypedGenericMap<KeyDT, Object>::iterator() const
{
    using GMEntryPoolT = pool::SimpleObjectPool<TypedGenericMapEntry<KeyDT, Object>>;
    using GMEntryPoolPtrT = boost::local_shared_ptr<GMEntryPoolT>;

    static thread_local GMEntryPoolPtrT entry_pool = MakeShared<GMEntryPoolT>();
    return entry_pool->allocate_shared(map_.begin(), map_.end(), map_.document().get(), ctr_holder_);
}

}}
