
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


template <typename DT>
ViewPtr<Datatype<DT>, true> Array<Value>::append(DTTViewType<DT> str)
{
    using DTCtr = Datatype<DT>;

    assert_not_null();
    assert_mutable();

    auto arena_dtc = doc_->arena()->allocate_tagged_object<typename DTCtr::ArenaDTContainer>(
        TypeHashV<DTCtr>,
        str
    );
    array_->push_back(*doc_->arena(), arena_dtc);

    return ViewPtr<DTCtr>(DTCtr(arena_dtc, doc_, ptr_holder_));
}

inline ViewPtr<Map<Varchar, Value>, true> Array<Value>::append_generic_map()
{
    using CtrT = Map<Varchar, Value>;

    assert_not_null();
    assert_mutable();

    auto arena_dtc = doc_->arena()->allocate_tagged_object<typename CtrT::ArenaMap>(
        TypeHashV<CtrT>
    );
    array_->push_back(*doc_->arena(), arena_dtc);

    return ViewPtr<CtrT>(CtrT(arena_dtc, doc_, ptr_holder_));
}


inline ViewPtr<Array<Value>, true> Array<Value>::append_generic_array()
{
    using CtrT = Array<Value>;

    assert_not_null();
    assert_mutable();

    auto arena_dtc = doc_->arena()->allocate_tagged_object<typename CtrT::ArenaArray>(
        TypeHashV<CtrT>
    );
    array_->push_back(*doc_->arena(), arena_dtc);

    return ViewPtr<CtrT>(CtrT(arena_dtc, doc_, ptr_holder_));
}

template <typename DT>
ViewPtr<Datatype<DT>, true> Array<Value>::set(uint64_t idx, DTTViewType<DT> str)
{
    using DTCtr = Datatype<DT>;

    assert_not_null();
    assert_mutable();

    if (MMA_LIKELY(idx < array_->size()))
    {
        auto arena_dtc = doc_->arena()->allocate_tagged_object<typename DTCtr::ArenaDTContainer>(
                    TypeHashV<DTCtr>,
                    str
        );

        array_->set(idx, arena_dtc);
        return ViewPtr<DTCtr>(DTCtr(arena_dtc, doc_, ptr_holder_));
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>::set(): {}::{}", idx, array_->size()).do_throw();
    }
}



inline ViewPtr<Map<Varchar, Value>, true> Array<Value>::set_generic_map(uint64_t idx)
{
    using CtrT = Map<Varchar, Value>;

    assert_not_null();
    assert_mutable();

    if (MMA_LIKELY(idx < array_->size()))
    {
        auto arena_dtc = doc_->arena()->allocate_tagged_object<typename CtrT::ArenaMap>(
            TypeHashV<CtrT>
        );

        array_->set(idx, arena_dtc);
        return ViewPtr<CtrT>(CtrT(arena_dtc, doc_, ptr_holder_));
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>::set(): {}::{}", idx, array_->size()).do_throw();
    }
}

inline ViewPtr<Array<Value>, true> Array<Value>::set_generic_array(uint64_t idx)
{
    using CtrT = Array<Value>;

    assert_not_null();
    assert_mutable();

    if (MMA_LIKELY(idx < array_->size()))
    {
        auto arena_dtc = doc_->arena()->allocate_tagged_object<typename CtrT::ArenaArray>(
            TypeHashV<CtrT>
        );

        array_->set(idx, arena_dtc);
        return ViewPtr<CtrT>(CtrT(arena_dtc, doc_, ptr_holder_));
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>::set(): {}::{}", idx, array_->size()).do_throw();
    }
}


}}
