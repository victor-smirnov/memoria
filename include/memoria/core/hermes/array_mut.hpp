
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

#include <memoria/core/hermes/array.hpp>
#include <memoria/core/hermes/document.hpp>

namespace memoria {
namespace hermes {

inline void Array<Value>::assert_mutable()
{
    if (MMA_UNLIKELY(doc_->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Array<Value> is immutable");
    }
}


inline ViewPtr<Datatype<Varchar>> Array<Value>::append_varchar(U8StringView str)
{
    using DTCtr = Datatype<Varchar>;

    assert_not_null();
    assert_mutable();

    auto arena_str = doc_->arena()->allocate_object<typename DTCtr::ArenaDTContainer>(str);
    array_->push_back(*doc_->arena(), (void*)arena_str);

    return ViewPtr<DTCtr>(DTCtr(arena_str, doc_, ptr_holder_));
}


}}
