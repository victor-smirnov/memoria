
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

#include <memoria/core/hermes/map.hpp>
#include <memoria/core/hermes/document.hpp>

namespace memoria {
namespace hermes {

inline void Map<Varchar, Value>::assert_mutable()
{
    if (MMA_UNLIKELY(doc_->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Map<String, Value> is immutable");
    }
}

template <typename DT>
inline ViewPtr<Datatype<DT>> Map<Varchar, Value>::put(U8StringView key, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    using KeyDTCtr = Datatype<Varchar>;
    using ArenaKeyDTCtr = typename KeyDTCtr::ArenaDTContainer;

    using ValueDTCtr = Datatype<DT>;
    using ArenaValueDTCtr = typename ValueDTCtr::ArenaDTContainer;


    auto arena = doc_->arena();
    auto arena_key = arena->template allocate_tagged_object<ArenaKeyDTCtr>(TypeHashV<KeyDTCtr>, key);
    auto arena_value = arena->template allocate_tagged_object<ArenaValueDTCtr>(TypeHashV<ValueDTCtr>, value);

    map_->put(*arena, arena_key, arena_value);

    return ViewPtr<ValueDTCtr>(ValueDTCtr(arena_value, doc_, ptr_holder_));
}


inline ViewPtr<Map<Varchar, Value>> Map<Varchar, Value>::put_generic_map(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    using KeyDTCtr = Datatype<Varchar>;
    using ArenaKeyDTCtr = typename KeyDTCtr::ArenaDTContainer;

    using ValueDTCtr = Map<Varchar, Value>;
    using ArenaValueDTCtr = typename ValueDTCtr::ArenaMap;


    auto arena = doc_->arena();
    auto arena_key = arena->template allocate_tagged_object<ArenaKeyDTCtr>(TypeHashV<KeyDTCtr>, key);
    auto arena_value = arena->template allocate_tagged_object<ArenaValueDTCtr>(TypeHashV<ValueDTCtr>);

    map_->put(*arena, arena_key, arena_value);

    return ViewPtr<ValueDTCtr>(ValueDTCtr(arena_value, doc_, ptr_holder_));
}

inline ViewPtr<Array<Value>> Map<Varchar, Value>::put_generic_array(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    using KeyDTCtr = Datatype<Varchar>;
    using ArenaKeyDTCtr = typename KeyDTCtr::ArenaDTContainer;

    using ValueDTCtr = Array<Value>;
    using ArenaValueDTCtr = typename ValueDTCtr::ArenaArray;


    auto arena = doc_->arena();
    auto arena_key = arena->template allocate_tagged_object<ArenaKeyDTCtr>(TypeHashV<KeyDTCtr>, key);
    auto arena_value = arena->template allocate_tagged_object<ArenaValueDTCtr>(TypeHashV<ValueDTCtr>);

    map_->put(*arena, arena_key, arena_value);

    return ViewPtr<ValueDTCtr>(ValueDTCtr(arena_value, doc_, ptr_holder_));
}


inline void Map<Varchar, Value>::remove(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    map_->remove(*(doc_->arena()), key);
}

}}
