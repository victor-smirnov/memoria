
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

inline ViewPtr<Datatype<Varchar>> Map<Varchar, Value>::put_varchar(U8StringView key, U8StringView value)
{
    assert_not_null();
    assert_mutable();

    using DTCtr = Datatype<Varchar>;
    using ArenaDTCtr = typename DTCtr::ArenaDTContainer;

    auto arena = doc_->arena();
    auto arena_key_str = arena->template allocate_tagged_object<ArenaDTCtr>(TypeHashV<DTCtr>, key);
    auto arena_value_str = arena->template allocate_tagged_object<ArenaDTCtr>(TypeHashV<DTCtr>, value);

    map_->put(*arena, arena_key_str, arena_value_str);

    return ViewPtr<DTCtr>(DTCtr(arena_value_str, doc_, ptr_holder_));
}

}}
